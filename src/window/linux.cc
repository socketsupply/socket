#include "window.hh"

static GtkTargetEntry droppableTypes[] = {
  { (char*) "text/uri-list", 0, 0 }
};

namespace SSC {
  struct WebViewJavaScriptAsyncContext {
    IPC::Router::ReplyCallback reply;
    IPC::Message message;
    Window *window;
  };

  Window::Window (App& app, WindowOptions opts) : app(app) , opts(opts) {
    setenv("GTK_OVERLAY_SCROLLING", "1", 1);
    this->accelGroup = gtk_accel_group_new();
    this->popupId = 0;
    this->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    this->popup = nullptr;

    this->bridge = new IPC::Bridge(app.core);
    this->bridge->router.dispatchFunction = [&app] (auto callback) {
      app.dispatch([callback] { callback(); });
    };

    this->bridge->router.evaluateJavaScriptFunction = [this] (auto js) {
      this->eval(js);
    };

    this->bridge->router.map("window.eval", [=](auto message, auto router, auto reply) {
      WindowManager* windowManager = app.getWindowManager();
      if (windowManager == nullptr) {
        // @TODO(jwerle): print warning
        return;
      }

      auto window = windowManager->getWindow(message.index);

      if (window == nullptr) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid window index given"}
        }});
      }

      auto value = message.get("value");
      auto ctx = new WebViewJavaScriptAsyncContext { reply, message, window };

      webkit_web_view_run_javascript(
        WEBKIT_WEB_VIEW(window->webview),
        value.c_str(),
        nullptr,
        [](GObject *object, GAsyncResult *res, gpointer data) {
          GError *error = nullptr;
          auto ctx = reinterpret_cast<WebViewJavaScriptAsyncContext*>(data);
          auto result = webkit_web_view_run_javascript_finish(
            WEBKIT_WEB_VIEW(ctx->window->webview),
            res,
            &error
          );

          if (!result) {
            ctx->reply(IPC::Result::Err { ctx->message, JSON::Object::Entries {
              {"code", error->code},
              {"message", String(error->message)}
            }});

            g_error_free(error);
            return;
          } else {
            auto value = webkit_javascript_result_get_js_value(result);

            if (
              jsc_value_is_null(value) ||
              jsc_value_is_array(value) ||
              jsc_value_is_object(value) ||
              jsc_value_is_number(value) ||
              jsc_value_is_string(value) ||
              jsc_value_is_function(value) ||
              jsc_value_is_undefined(value) ||
              jsc_value_is_constructor(value)
            ) {
              auto context = jsc_value_get_context(value);
              auto string = jsc_value_to_string(value);
              auto exception = jsc_context_get_exception(context);
              auto json = JSON::Any {};

              if (exception) {
                auto message = jsc_exception_get_message(exception);

                if (message == nullptr) {
                  message = "An unknown error occured";
                }

                ctx->reply(IPC::Result::Err { ctx->message, JSON::Object::Entries {
                  {"message", String(message)}
                }});
              } else if (string) {
                ctx->reply(IPC::Result { ctx->message.seq, ctx->message, String(string) });
              }

              if (string) {
                g_free(string);
              }
            } else {
              ctx->reply(IPC::Result::Err { ctx->message, JSON::Object::Entries {
                {"message", "Error: An unknown JavaScript evaluation error has occurred"}
              }});
            }
          }

          webkit_javascript_result_unref(result);
        },
        ctx
      );
    });

    if (opts.resizable) {
      gtk_window_set_default_size(GTK_WINDOW(window), opts.width, opts.height);
      gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    } else {
      gtk_widget_set_size_request(window, opts.width, opts.height);
    }

    gtk_window_set_resizable(GTK_WINDOW(window), opts.resizable);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    WebKitUserContentManager* cm = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(cm, "external");

    g_signal_connect(
      cm,
      "script-message-received::external",
      G_CALLBACK(+[](
        WebKitUserContentManager* cm,
        WebKitJavascriptResult* result,
        gpointer ptr
      ) {
        auto window = static_cast<Window*>(ptr);
        auto value = webkit_javascript_result_get_js_value(result);
        auto valueString = jsc_value_to_string(value);
        auto str = String(valueString);

        char *buf = nullptr;
        size_t bufsize = 0;

        // 'b5' for 'buffer'
        if (str.size() >= 2 && str.at(0) == 'b' && str.at(1) == '5') {
          size_t size = 0;
          auto bytes = jsc_value_to_string_as_bytes(value);
          auto data = (char *) g_bytes_get_data(bytes, &size);

          if (size > 6) {
            size_t offset = 2 + 4 + 20; // buf offset
            auto index = new char[4]{0};
            auto seq = new char[20]{0};

            decodeUTF8(index, data + 2, 4);
            decodeUTF8(seq, data + 2 + 4, 20);

            buf = new char[size - offset]{0};
            bufsize = decodeUTF8(buf, data + offset, size - offset);

            str = String("ipc://buffer.map?index=") + index + "&seq=" + seq;

            delete [] index;
            delete [] seq;
          }

          g_bytes_unref(bytes);
        }

        if (!window->bridge->route(str, buf, bufsize)) {
          if (window->onMessage != nullptr) {
            window->onMessage(str);
          }
        }

        g_free(valueString);
        delete [] buf;
      }),
      this
    );

    webview = webkit_web_view_new_with_user_content_manager(cm);

    g_signal_connect(
      G_OBJECT(webview),
      "decide-policy",
      G_CALLBACK(+[](
        WebKitWebView* webview,
        WebKitPolicyDecision* decision,
        WebKitPolicyDecisionType decisionType,
        gpointer userData
      ) {
        if (decisionType != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
          webkit_policy_decision_use(decision);
          return true;
        }

        auto nav = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
        auto action = webkit_navigation_policy_decision_get_navigation_action(nav);
        auto req = webkit_navigation_action_get_request(action);
        auto uri = String(webkit_uri_request_get_uri(req));

        if (uri.find("file://") != 0 && uri.find("http://localhost") != 0 && uri.find("socket:") != 0) {
          webkit_policy_decision_ignore(decision);
          return false;
        }

        return true;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "load-changed",
      G_CALLBACK(+[](WebKitWebView* wv, WebKitLoadEvent event, gpointer arg) {
        auto *window = static_cast<Window*>(arg);
        if (event == WEBKIT_LOAD_STARTED) {
          window->app.isReady = false;
        }

        if (event == WEBKIT_LOAD_FINISHED) {
          window->app.isReady = true;
        }
      }),
      this
    );

    // Calling gtk_drag_source_set interferes with text selection
    /* gtk_drag_source_set(
      webview,
      (GdkModifierType)(GDK_BUTTON1_MASK | GDK_BUTTON2_MASK),
      droppableTypes,
      G_N_ELEMENTS(droppableTypes),
      GDK_ACTION_COPY
    ); */

    /* gtk_drag_dest_set(
      webview,
      GTK_DEST_DEFAULT_ALL,
      droppableTypes,
      1,
      GDK_ACTION_MOVE
    ); */

    g_signal_connect(
      G_OBJECT(webview),
      "drag-begin",
      G_CALLBACK(+[](GtkWidget *wv, GdkDragContext *context, gpointer arg) {
        auto *w = static_cast<Window*>(arg);
        w->isDragInvokedInsideWindow = true;

        GdkDevice* device;
        gint wx;
        gint wy;
        gint x;
        gint y;

        device = gdk_drag_context_get_device(context);
        gdk_device_get_window_at_position(device, &x, &y);
        gdk_device_get_position(device, 0, &wx, &wy);

        String js(
          "(() => {"
          "  let el = null;"
          "  try { el = document.elementFromPoint(" + std::to_string(x) + "," + std::to_string(y) + "); }"
          "  catch (err) { console.error(err.stack || err.message || err); }"
          "  if (!el) return;"
          "  const found = el.matches('[data-src]') ? el : el.closest('[data-src]');"
          "  return found && found.dataset.src"
          "})()"
        );

        webkit_web_view_run_javascript(
          WEBKIT_WEB_VIEW(wv),
          js.c_str(),
          nullptr,
          [](GObject* src, GAsyncResult* result, gpointer arg) {
            auto *w = static_cast<Window*>(arg);
            if (!w) return;

            GError* error = NULL;
            WebKitJavascriptResult* wkr = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(src), result, &error);
            if (!wkr || error) return;

            auto* value = webkit_javascript_result_get_js_value(wkr);
            if (!jsc_value_is_string(value)) return;

            JSCException *exception;
            gchar *str_value = jsc_value_to_string(value);


            w->draggablePayload = split(str_value, ';');
            exception = jsc_context_get_exception(jsc_value_get_context(value));

            g_free(str_value);
          },
          w
        );
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "focus",
      G_CALLBACK(+[](
        GtkWidget *wv,
        GtkDirectionType direction,
        gpointer arg)
      {
        auto *w = static_cast<Window*>(arg);
        if (!w) return;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "drag-data-get",
      G_CALLBACK(+[](
        GtkWidget *wv,
        GdkDragContext *context,
        GtkSelectionData *data,
        guint info,
        guint time,
        gpointer arg)
      {
        auto *w = static_cast<Window*>(arg);
        if (!w) return;

        if (w->isDragInvokedInsideWindow) {
          // FIXME: Once, write a single tmp file `/tmp/{i64}.download` and
          // add it to the draggablePayload, then start the fanotify watcher
          // for that particular file.
          return;
        }

        if (w->draggablePayload.size() == 0) return;

        gchar* uris[w->draggablePayload.size() + 1];
        int i = 0;

        for (auto& file : w->draggablePayload) {
          if (file[0] == '/') {
            // file system paths must be proper URIs
            file = String("file://" + file);
          }
          uris[i++] = strdup(file.c_str());
        }

        uris[i] = NULL;

        gtk_selection_data_set_uris(data, uris);
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "drag-motion",
      G_CALLBACK(+[](
        GtkWidget *wv,
        GdkDragContext *context,
        gint x,
        gint y,
        guint32 time,
        gpointer arg)
      {
        auto *w = static_cast<Window*>(arg);
        if (!w) return;

        w->dragLastX = x;
        w->dragLastY = y;

        // char* target_uri = g_file_get_uri(drag_info->target_location);

        int count = w->draggablePayload.size();
        bool inbound = !w->isDragInvokedInsideWindow;

        // w->eval(getEmitToRenderProcessJavaScript("dragend", "{}"));

        // TODO wtf we get a toaster instead of actual focus
        gtk_window_present(GTK_WINDOW(w->window));
        // gdk_window_focus(GDK_WINDOW(w->window), nullptr);

        String json = (
          "{\"count\":" + std::to_string(count) + ","
          "\"inbound\":" + (inbound ? "true" : "false") + ","
          "\"x\":" + std::to_string(x) + ","
          "\"y\":" + std::to_string(y) + "}"
        );

        w->eval(getEmitToRenderProcessJavaScript("drag", json));
      }),
      this
    );

    // https://wiki.gnome.org/Newcomers/XdsTutorial
    // https://wiki.gnome.org/action/show/Newcomers/OldDragNDropTutorial?action=show&redirect=Newcomers%2FDragNDropTutorial

    g_signal_connect(
      G_OBJECT(webview),
      "drag-end",
      G_CALLBACK(+[](GtkWidget *wv, GdkDragContext *context, gpointer arg) {
        auto *w = static_cast<Window*>(arg);
        if (!w) return;

        w->isDragInvokedInsideWindow = false;
        w->draggablePayload.clear();
        w->eval(getEmitToRenderProcessJavaScript("dragend", "{}"));
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(window),
      "button-release-event",
      G_CALLBACK(+[](GtkWidget* window, GdkEventButton event, gpointer arg) {
        auto *w = static_cast<Window*>(arg);
        if (!w) return;

        /**
         * Calling w->eval() inside button-release-event causes
         * a Segmentation Fault on Ubuntu 22, but works fine on
         * other linuxes like Ubuntu 20.
         *
         * The dragend feature causes the application to crash in
         * all operations including non drag-drop and causes applications
         * that do not use drag and drop at all to crash.
         *
         * So disabling this experimental linux dragdrop code for now.
         */

        // w->isDragInvokedInsideWindow = false;
        // w->eval(getEmitToRenderProcessJavaScript("dragend", "{}"));
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "drag-data-received",
      G_CALLBACK(+[](
        GtkWidget* wv,
        GdkDragContext* context,
        gint x,
        gint y,
        GtkSelectionData *data,
        guint info,
        guint time,
        gpointer arg)
      {
        auto* w = static_cast<Window*>(arg);
        if (!w) return;

        gtk_drag_dest_add_uri_targets(wv);
        gchar** uris = gtk_selection_data_get_uris(data);
        int len = gtk_selection_data_get_length(data) - 1;
        if (!uris) return;

        auto v = &w->draggablePayload;

        for(size_t n = 0; uris[n] != nullptr; n++) {
          gchar* src = g_filename_from_uri(uris[n], nullptr, nullptr);
          if (src) {
            auto s = String(src);
            if (std::find(v->begin(), v->end(), s) == v->end()) {
              v->push_back(s);
            }
            g_free(src);
          }
        }
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "drag-drop",
      G_CALLBACK(+[](
        GtkWidget* widget,
        GdkDragContext* context,
        gint x,
        gint y,
        guint time,
        gpointer arg)
      {
        auto* w = static_cast<Window*>(arg);
        auto count = w->draggablePayload.size();
        JSON::Array files;

        for (int i = 0 ; i < count; ++i) {
          files[i] = w->draggablePayload[i];
        }

        JSON::Object json;
        json["files"] = files;
        json["x"] = x;
        json["y"] = y;

        JSON::Object options;
        options["bubbles"] = true;

        w->eval(getEmitToRenderProcessJavaScript(
          "dropin",
          json.str(),
          "document.elementFromPoint(" + std::to_string(x) + "," + std::to_string(y) + ")",
          options
        ));

        w->draggablePayload.clear();
        w->eval(getEmitToRenderProcessJavaScript("dragend", "{}"));
        gtk_drag_finish(context, TRUE, TRUE, time);
        return TRUE;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(window),
      "destroy",
      G_CALLBACK(+[](GtkWidget*, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        w->close(0);
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(window),
      "delete-event",
      G_CALLBACK(+[](GtkWidget* widget, GdkEvent*, gpointer arg) {
        auto* w = static_cast<Window*>(arg);

        if (w->opts.canExit == false) {
          w->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
          return gtk_widget_hide_on_delete(widget);
        }

        w->close(0);
        return FALSE;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(window),
      "size-allocate", // https://docs.gtk.org/gtk3/method.Window.get_size.html
      G_CALLBACK(+[](GtkWidget* widget,GtkAllocation *allocation, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        gtk_window_get_size(GTK_WINDOW(widget), &w->width, &w->height);
      }),
      this
    );

    String preload = ToString(createPreload(opts));

    WebKitUserContentManager *manager =
      webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));

    webkit_user_content_manager_add_script(
      manager,
      webkit_user_script_new(
        preload.c_str(),
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr,
        nullptr
      )
    );

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_zoom_text_only(settings, false);

    GdkRGBA rgba = {0};
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), &rgba);

    if (this->opts.debug) {
      webkit_settings_set_enable_developer_extras(settings, true);
    }

    webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
    webkit_settings_set_allow_file_access_from_file_urls(settings, true);

    // webkit_settings_set_allow_top_navigation_to_data_urls(settings, true);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_end(GTK_BOX(vbox), webview, true, true, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_add_events(window, GDK_ALL_EVENTS_MASK);

    gtk_widget_grab_focus(GTK_WIDGET(webview));
  }

  ScreenSize Window::getScreenSize () {
    auto list = gtk_window_list_toplevels();
    int width = 0;
    int height = 0;

    if (list != nullptr) {
      for (auto entry = list; entry != nullptr; entry = entry->next) {
        auto widget = (GtkWidget*) entry->data;
        auto window = GTK_WINDOW(widget);

        if (window != nullptr) {
          auto geometry = GdkRectangle {};
          auto display = gtk_widget_get_display(widget);
          auto monitor = gdk_display_get_monitor_at_window(
            display,
            gtk_widget_get_window(widget)
          );

          gdk_monitor_get_geometry(monitor, &geometry);

          width = (int) geometry.width;
          height = (int) geometry.height;

          break;
        }
      }

      g_list_free(list);
    }

    if (!height || !width) {
      auto geometry = GdkRectangle {};
      auto display = gdk_display_get_default();
      auto monitor = gdk_display_get_primary_monitor(display);

      gdk_monitor_get_workarea(monitor, &geometry);
      width = (int) geometry.width;
      height = (int) geometry.height;
    }

    return ScreenSize { height, width };
  }

  void Window::eval (const String& source) {
    auto webview = this->webview;
    this->app.dispatch([=, this] {
      webkit_web_view_run_javascript(
        WEBKIT_WEB_VIEW(this->webview),
        String(source).c_str(),
        nullptr,
        nullptr,
        nullptr
      );
    });
  }

  void Window::show () {
    gtk_widget_realize(this->window);

    if (this->opts.headless == false) {
      gtk_widget_show_all(this->window);
      gtk_window_present(GTK_WINDOW(this->window));
    }
  }

  void Window::hide () {
    gtk_widget_realize(this->window);
    gtk_widget_hide(this->window);
    this->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    GdkRGBA color;
    color.red = r / 255.0;
    color.green = g / 255.0;
    color.blue = b / 255.0;
    color.alpha = a;

    gtk_widget_realize(this->window);
    gtk_widget_override_background_color(
      this->window, GTK_STATE_FLAG_NORMAL, &color
    );
  }

  void Window::showInspector () {
    // this->webview->inspector.show();
  }

  void Window::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  void Window::kill () {
    // gtk releases objects automatically.
  }

  void Window::close (int code) {
    if (opts.canExit) {
      this->exit(code);
    }
  }

  void Window::navigate (const String &seq, const String &url) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), url.c_str());

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  SSC::String Window::getTitle () {
    auto title = gtk_window_get_title(GTK_WINDOW(window));
    return String(title != nullptr ? title : "");
  }

  void Window::setTitle (const String &s) {
    gtk_widget_realize(window);
    gtk_window_set_title(GTK_WINDOW(window), s.c_str());
  }

  int Window::openExternal (const String& url) {
    gtk_widget_realize(window);
    return gtk_show_uri_on_window(GTK_WINDOW(window), url.c_str(), GDK_CURRENT_TIME, nullptr);
  }

  void Window::about () {
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);

    GtkWidget *body = gtk_dialog_get_content_area(GTK_DIALOG(GTK_WINDOW(dialog)));
    GtkContainer *content = GTK_CONTAINER(body);

    String imgPath = "/usr/share/icons/hicolor/256x256/apps/" +
      app.appData["build_name"] +
      ".png";

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
      imgPath.c_str(),
      60,
      60,
      true,
      nullptr
    );

    GtkWidget *img = gtk_image_new_from_pixbuf(pixbuf);
    gtk_widget_set_margin_top(img, 20);
    gtk_widget_set_margin_bottom(img, 20);

    gtk_box_pack_start(GTK_BOX(content), img, false, false, 0);

    String title_value(app.appData["build_name"] + " v" + app.appData["meta_version"]);
    String version_value("Built with ssc v" + SSC::VERSION_FULL_STRING);

    GtkWidget *label_title = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_title), title_value.c_str());
    gtk_container_add(content, label_title);

    GtkWidget *label_op_version = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_op_version), version_value.c_str());
    gtk_container_add(content, label_op_version);

    GtkWidget *label_copyright = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_copyright), app.appData["meta_copyright"].c_str());
    gtk_container_add(content, label_copyright);

    g_signal_connect(
      dialog,
      "response",
      G_CALLBACK(gtk_widget_destroy),
      nullptr
    );

    gtk_widget_show_all(body);
    gtk_widget_show_all(dialog);
    gtk_window_set_title(GTK_WINDOW(dialog), "About");

    gtk_dialog_run(GTK_DIALOG(dialog));
  }

  ScreenSize Window::getSize () {
    return ScreenSize { this->height, this->width };
  }

  void Window::setSize (int width, int height, int hints) {
    gtk_widget_realize(window);
    gtk_window_set_resizable(GTK_WINDOW(window), hints != WINDOW_HINT_FIXED);

    if (hints == WINDOW_HINT_NONE) {
      gtk_window_resize(GTK_WINDOW(window), width, height);
    } else if (hints == WINDOW_HINT_FIXED) {
      gtk_widget_set_size_request(window, width, height);
    } else {
      GdkGeometry g;
      g.min_width = g.max_width = width;
      g.min_height = g.max_height = height;

      GdkWindowHints h = (hints == WINDOW_HINT_MIN
        ? GDK_HINT_MIN_SIZE
        : GDK_HINT_MAX_SIZE
      );

      gtk_window_set_geometry_hints(GTK_WINDOW(window), nullptr, &g, h);
    }

    this->width = width;
    this->height = height;
  }

  void Window::setSystemMenu (const String &seq, const String &value) {
    if (value.empty()) return void(0);

    auto menu = String(value);

    if (menubar == nullptr) {
      menubar = gtk_menu_bar_new();
    } else {
      GList *iter;
      GList *children = gtk_container_get_children(GTK_CONTAINER(menubar));

      for (iter = children; iter != nullptr; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
      }
      g_list_free(children);
    }

    // deserialize the menu
    menu = replace(menu, "%%", "\n");

    // split on ;
    auto menus = split(menu, ';');

    for (auto m : menus) {
      auto menu = split(m, '\n');
      auto line = trim(menu[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];
      GtkWidget *subMenu = gtk_menu_new();
      GtkWidget *menuItem = gtk_menu_item_new_with_label(menuTitle.c_str());

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.empty()) continue;
        auto parts = split(line, ':');
        auto title = parts[0];
        String key = "";

        GtkWidget *item;

        if (parts[0].find("---") != -1) {
          item = gtk_separator_menu_item_new();
        } else {
          item = gtk_menu_item_new_with_label(title.c_str());

          if (parts.size() > 1) {
            auto value = trim(parts[1]);
            key = value == "_" ? "" : value;

            if (key.size() > 0) {
              auto accelerator = split(parts[1], '+');
              key = trim(parts[1]) == "_" ? "" : trim(accelerator[0]);

              GdkModifierType mask = (GdkModifierType)(0);
              bool isShift = String("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;

              if (accelerator.size() > 1) {
                if (accelerator[1].find("Meta") != -1) {
                  mask = (GdkModifierType)(mask | GDK_META_MASK);
                }

                if (accelerator[1].find("CommandOrControl") != -1) {
                  mask = (GdkModifierType)(mask | GDK_CONTROL_MASK);
                } else if (accelerator[1].find("Control") != -1) {
                  mask = (GdkModifierType)(mask | GDK_CONTROL_MASK);
                }

                if (accelerator[1].find("Alt") != -1) {
                  mask = (GdkModifierType)(mask | GDK_MOD1_MASK);
                }
              }

              if (isShift) {
                mask = (GdkModifierType)(mask | GDK_SHIFT_MASK);
              }

              gtk_widget_add_accelerator(
                item,
                "activate",
                accelGroup,
                (guint) key[0],
                mask,
                GTK_ACCEL_VISIBLE
              );

              gtk_widget_show(item);
            }
          }

          g_signal_connect(
            G_OBJECT(item),
            "activate",
            G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
              auto w = static_cast<Window*>(arg);
              auto title = gtk_menu_item_get_label(GTK_MENU_ITEM(t));
              auto parent = gtk_widget_get_name(t);

              if (String(title).find("About") == 0) {
                return w->about();
              }

              if (String(title).find("Quit") == 0) {
                return w->exit(0);
              }

              w->eval(getResolveMenuSelectionJavaScript("0", title, parent));
            }),
            this
          );

        }

        gtk_widget_set_name(item, menuTitle.c_str());
        gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), item);
      }

      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), subMenu);
      gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuItem);
    }

    gtk_box_pack_start(GTK_BOX(vbox), menubar, false, false, 0);
    gtk_widget_show_all(menubar);

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // @TODO(): provide impl
  }

  void Window::closeContextMenu () {
    if (popupId > 0) {
      popupId = 0;
      auto seq = std::to_string(popupId);
      closeContextMenu(seq);
    }
  }

  void Window::closeContextMenu (const String &seq) {
    if (popup != nullptr) {
      auto ptr = popup;
      popup = nullptr;
      closeContextMenu(ptr, seq);
    }
  }

  void Window::closeContextMenu (
    GtkWidget *popupMenu,
    const String &seq
  ) {
    if (popupMenu != nullptr) {
      gtk_menu_popdown((GtkMenu *) popupMenu);
      gtk_widget_destroy(popupMenu);
      this->eval(getResolveMenuSelectionJavaScript(seq, "", "contextMenu"));
    }
  }

  void Window::setContextMenu (
    const String &seq,
    const String &value
  ) {
    closeContextMenu();

    // members
    popup = gtk_menu_new();

    try {
      popupId = std::stoi(seq);
    } catch (...) {
      popupId = 0;
    }

    auto menuData = replace(value, "_", "\n");
    auto menuItems = split(menuData, '\n');

    for (auto itemData : menuItems) {
      if (trim(itemData).size() == 0) {
        continue;
      }

      if (itemData.find("---") != -1) {
        auto *item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);
        continue;
      }

      auto pair = split(itemData, ':');
      auto meta = String(seq + ";" + pair[0].c_str());
      auto *item = gtk_menu_item_new_with_label(pair[0].c_str());

      g_signal_connect(
        G_OBJECT(item),
        "activate",
        G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
          auto window = static_cast<Window*>(arg);
          auto label = gtk_menu_item_get_label(GTK_MENU_ITEM(t));
          auto title = String(label);
          auto meta = gtk_widget_get_name(t);
          auto pair = split(meta, ';');
          auto seq = pair[0];

          window->eval(getResolveMenuSelectionJavaScript(seq, title, "contextMenu"));
        }),
        this
      );

      gtk_widget_set_name(item, meta.c_str());
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);
    }

    GdkRectangle rect;
    gint x, y;

    auto win = GDK_WINDOW(gtk_widget_get_window(window));
    auto seat = gdk_display_get_default_seat(gdk_display_get_default());
    auto event = gdk_event_new(GDK_BUTTON_PRESS);
    auto mouse_device = gdk_seat_get_pointer(seat);

    gdk_window_get_device_position(win, mouse_device, &x, &y, nullptr);
    gdk_event_set_device(event, mouse_device);

    event->button.send_event = 1;
    event->button.button = GDK_BUTTON_SECONDARY;
    event->button.window = GDK_WINDOW(g_object_ref(win));
    event->button.time = GDK_CURRENT_TIME;

    rect.height = 0;
    rect.width = 0;
    rect.x = x - 1;
    rect.y = y - 1;

    gtk_widget_add_events(popup, GDK_ALL_EVENTS_MASK);
    gtk_widget_set_can_focus(popup, true);
    gtk_widget_show_all(popup);
    gtk_widget_grab_focus(popup);

    gtk_menu_popup_at_rect(
      GTK_MENU(popup),
      win,
      &rect,
      GDK_GRAVITY_SOUTH_WEST,
      GDK_GRAVITY_NORTH_WEST,
      event
    );
  }

  void Window::openDialog (
    const String& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const String& defaultPath,
    const String& title,
    const String& defaultName
  ) {
    const guint SELECT_RESPONSE = 0;
    GtkFileChooserAction action;
    GtkFileChooser *chooser;
    GtkFileFilter *filter;
    GtkWidget *dialog;

    if (isSave) {
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
    } else {
      action = GTK_FILE_CHOOSER_ACTION_OPEN;
    }

    if (!allowFiles && allowDirs) {
      action = (GtkFileChooserAction) (action | GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    }

    gtk_init_check(nullptr, nullptr);

    String dialogTitle = isSave ? "Save File" : "Open File";
    if (title.size() > 0) {
      dialogTitle = title;
    }

    dialog = gtk_file_chooser_dialog_new(
      dialogTitle.c_str(),
      nullptr,
      action,
      "_Cancel",
      GTK_RESPONSE_CANCEL,
      nullptr
    );

    chooser = GTK_FILE_CHOOSER(dialog);

    if (!allowDirs) {
      if (isSave) {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Save", GTK_RESPONSE_ACCEPT);
      } else {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Open", GTK_RESPONSE_ACCEPT);
      }
    }

    if (allowMultiple || allowDirs) {
      gtk_dialog_add_button(GTK_DIALOG(dialog), "Select", SELECT_RESPONSE);
    }

    // if (FILE_DIALOG_OVERWRITE_CONFIRMATION) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);
    // }

    if ((!isSave || allowDirs) && allowMultiple) {
      gtk_file_chooser_set_select_multiple(chooser, true);
    }

    if (defaultPath.size() > 0) {
      auto status = fs::status(defaultPath);

      if (fs::exists(status)) {
        if (fs::is_directory(status)) {
          gtk_file_chooser_set_current_folder(chooser, defaultPath.c_str());
        } else {
          gtk_file_chooser_set_filename(chooser, defaultPath.c_str());
        }
      }
    }

    if (defaultName.size() > 0) {
      if ((!allowFiles && allowDirs) || isSave) {
        gtk_file_chooser_set_current_name(chooser, defaultName.c_str());
      } else {
        gtk_file_chooser_set_current_folder(chooser, defaultName.c_str());
      }
    }

    guint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_ACCEPT && response != SELECT_RESPONSE) {
      this->resolvePromise(seq, "0", "null");
      gtk_widget_destroy(dialog);
      return;
    }

    // TODO (@heapwolf): validate multi-select

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }

    String result("");
    GSList* filenames = gtk_file_chooser_get_filenames(chooser);
    int i = 0;

    for (int i = 0; filenames != nullptr; ++i) {
      gchar* file = (gchar*) filenames->data;
      result += (i ? "\\n" : "");
      result += String(file);
      filenames = filenames->next;
    }

    g_slist_free(filenames);

    auto wrapped = String("\"" + String(result) + "\"");
    this->resolvePromise(seq, "0", encodeURIComponent(wrapped));
    gtk_widget_destroy(dialog);
  }
}
