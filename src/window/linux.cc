#include "window.hh"

static GtkTargetEntry droppableTypes[] = {
  { (char*) "text/uri-list", 0, 0 }
};

#define DEFAULT_MONITOR_WIDTH 720
#define DEFAULT_MONITOR_HEIGHT 364

namespace SSC {
  struct WebViewJavaScriptAsyncContext {
    IPC::Router::ReplyCallback reply;
    IPC::Message message;
    Window *window;
  };

  Window::Window (App& app, WindowOptions opts)
    : app(app),
      opts(opts),
      hotkey(this)
  {
    static auto userConfig = SSC::getUserConfig();
    setenv("GTK_OVERLAY_SCROLLING", "1", 1);
    this->accelGroup = gtk_accel_group_new();
    this->popupId = 0;
    this->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    this->popup = nullptr;

    gtk_widget_set_can_focus(GTK_WIDGET(this->window), true);

    this->index = this->opts.index;
    this->bridge = new IPC::Bridge(app.core);

    this->hotkey.init(this->bridge);

    this->bridge->router.dispatchFunction = [&app] (auto callback) {
      app.dispatch([callback] { callback(); });
    };

    this->bridge->router.evaluateJavaScriptFunction = [this] (auto js) {
      this->eval(js);
    };

    this->bridge->router.map("window.eval", [=, &app](auto message, auto router, auto reply) {
      WindowManager* windowManager = app.getWindowManager();
      if (windowManager == nullptr) {
        // @TODO(jwerle): print warning
        return;
      }

      auto window = windowManager->getWindow(message.index);
      static auto userConfig = SSC::getUserConfig();

      if (userConfig["application_agent"] == "true") {
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
      }

      if (window == nullptr) {
        return reply(IPC::Result::Err { message, JSON::Object::Entries {
          {"message", "Invalid window index given"}
        }});
      }

      auto value = message.get("value");
      auto ctx = new WebViewJavaScriptAsyncContext { reply, message, window };

      webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(window->webview),
        value.c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        [](GObject *object, GAsyncResult *res, gpointer data) {
          GError *error = nullptr;
          auto ctx = reinterpret_cast<WebViewJavaScriptAsyncContext*>(data);
          auto value = webkit_web_view_evaluate_javascript_finish(
            WEBKIT_WEB_VIEW(ctx->window->webview),
            res,
            &error
          );

          if (!value) {
            ctx->reply(IPC::Result::Err { ctx->message, JSON::Object::Entries {
              {"code", error->code},
              {"message", String(error->message)}
            }});

            g_error_free(error);
            return;
          } else {
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

    static auto userConfig = SSC::getUserConfig();
    auto webContext = this->bridge->router.webkitWebContext;
    auto cookieManager = webkit_web_context_get_cookie_manager(webContext);
    auto settings = webkit_settings_new();
    auto policies = webkit_website_policies_new_with_policies(
      "autoplay", userConfig["permission_allow_autoplay"] != "false" ? WEBKIT_AUTOPLAY_ALLOW : WEBKIT_AUTOPLAY_DENY,
      NULL
    );

    webview = GTK_WIDGET(WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
      "web-context", webContext,
      "settings", settings,
      "user-content-manager", cm,
      "website-policies", policies,
      NULL
    )));

    gtk_widget_set_can_focus(GTK_WIDGET(webview), true);

    webkit_cookie_manager_set_accept_policy(cookieManager, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    g_signal_connect(
      G_OBJECT(webContext),
      "initialize-notification-permissions",
      G_CALLBACK(+[](
        WebKitWebContext* webContext,
        gpointer userData
      ) {
        static auto userConfig = SSC::getUserConfig();
        static const auto bundleIdentifier = userConfig["meta_bundle_identifier"];

        auto uri = "socket://" + bundleIdentifier;
        auto origin = webkit_security_origin_new_for_uri(uri.c_str());
        GList* allowed = nullptr;
        GList* disallowed = nullptr;

        webkit_security_origin_ref(origin);

        if (origin && allowed && disallowed) {
          if (userConfig["permissions_allow_notifications"] == "false") {
            disallowed = g_list_append(disallowed, (gpointer) origin);
          } else {
            allowed = g_list_append(allowed, (gpointer) origin);
          }

          if (allowed && disallowed) {
            webkit_web_context_initialize_notification_permissions(
              webContext,
              allowed,
              disallowed
            );
          }
        }

        if (allowed) {
          g_list_free(allowed);
        }

        if (disallowed) {
          g_list_free(disallowed);
        }

        if (origin) {
          webkit_security_origin_unref(origin);
        }
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "show-notification",
      G_CALLBACK(+[](
        WebKitWebView* webview,
        WebKitNotification* notification,
        gpointer userData
      ) -> bool {
        static auto userConfig = SSC::getUserConfig();
        return userConfig["permissions_allow_notifications"] == "false";
      }),
      this
    );

    // handle `navigator.permissions.query()`
    g_signal_connect(
      G_OBJECT(webview),
      "query-permission-state",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPermissionStateQuery* query,
        gpointer user_data
      ) -> bool {
        static auto userConfig = SSC::getUserConfig();
        auto name = String(webkit_permission_state_query_get_name(query));

        if (name == "geolocation") {
          webkit_permission_state_query_finish(
            query,
            userConfig["permissions_allow_geolocation"] == "false"
              ? WEBKIT_PERMISSION_STATE_DENIED
              : WEBKIT_PERMISSION_STATE_PROMPT
          );
        }

        if (name == "notifications") {
          webkit_permission_state_query_finish(
            query,
            userConfig["permissions_allow_notifications"] == "false"
              ? WEBKIT_PERMISSION_STATE_DENIED
              : WEBKIT_PERMISSION_STATE_PROMPT
          );
        }

        webkit_permission_state_query_finish(
          query,
          WEBKIT_PERMISSION_STATE_PROMPT
        );
        return true;
      })),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "permission-request",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPermissionRequest *request,
        gpointer userData
      ) -> bool {
        Window* window = reinterpret_cast<Window*>(userData);
        static auto userConfig = SSC::getUserConfig();
        auto result = false;
        String name = "";
        String description = "{{meta_title}} would like permission to use a an unknown feature.";

        if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request)) {
          name = "geolocation";
          result = userConfig["permissions_allow_geolocation"] != "false";
          description = "{{meta_title}} would like access to your location.";
        } else if (WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(request)) {
          name = "notifications";
          result = userConfig["permissions_allow_notifications"] != "false";
          description = "{{meta_title}} would like display notifications.";
        } else if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
          if (webkit_user_media_permission_is_for_audio_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            name = "microphone";
            result = userConfig["permissions_allow_microphone"] == "false";
            description = "{{meta_title}} would like access to your microphone.";
          }

          if (webkit_user_media_permission_is_for_video_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            name = "camera";
            result = userConfig["permissions_allow_camera"] == "false";
            description = "{{meta_title}} would like access to your camera.";
          }

          result = userConfig["permissions_allow_user_media"] == "false";
        } else if (WEBKIT_IS_WEBSITE_DATA_ACCESS_PERMISSION_REQUEST(request)) {
          name = "storage-access";
          result = userConfig["permissions_allow_data_access"] != "false";
          description = "{{meta_title}} would like access to local storage.";
        } else if (WEBKIT_IS_DEVICE_INFO_PERMISSION_REQUEST(request)) {
          result = userConfig["permissions_allow_device_info"] != "false";
          description = "{{meta_title}} would like access to your device information.";
        } else if (WEBKIT_IS_MEDIA_KEY_SYSTEM_PERMISSION_REQUEST(request)) {
          result = userConfig["permissions_allow_media_key_system"] != "false";
          description = "{{meta_title}} would like access to your media key system.";
        }

        if (result) {
          auto title = userConfig["meta_title"];
          GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(window->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            "%s",
            tmpl(description, userConfig).c_str()
          );

          gtk_widget_show(dialog);
          if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) {
            webkit_permission_request_allow(request);
          } else {
            webkit_permission_request_deny(request);
          }

          gtk_widget_destroy(dialog);
        } else {
          webkit_permission_request_deny(request);
        }

        if (name.size() > 0) {
          JSON::Object::Entries json = JSON::Object::Entries {
            {"name", name},
            {"state", result ? "granted" : "denied"}
          };
        }

        return result;
      })),
      this
    );

    g_signal_connect(
      G_OBJECT(webview),
      "decide-policy",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPolicyDecision* decision,
        WebKitPolicyDecisionType decisionType,
        gpointer userData
      ) {
        static const auto devHost = SSC::getDevHost();
        auto window = static_cast<Window*>(userData);

        if (decisionType != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
          webkit_policy_decision_use(decision);
          return true;
        }

        const auto nav = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
        const auto action = webkit_navigation_policy_decision_get_navigation_action(nav);
        const auto req = webkit_navigation_action_get_request(action);
        const auto uri = String(webkit_uri_request_get_uri(req));

        if (uri.starts_with(userConfig["meta_application_protocol"])) {
          webkit_policy_decision_ignore(decision);

          if (window != nullptr && window->bridge != nullptr) {
            SSC::JSON::Object json = SSC::JSON::Object::Entries {
              {"url", uri }
            };

            window->bridge->router.emit("applicationurl", json.str());
          }

          return false;
        }

        if (uri.find("socket:") != 0 && uri.find(devHost) != 0) {
          webkit_policy_decision_ignore(decision);
          return false;
        }

        return true;
      })),
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

        webkit_web_view_evaluate_javascript(
          WEBKIT_WEB_VIEW(wv),
          js.c_str(),
          -1,
          nullptr,
          nullptr,
          nullptr,
          [](GObject* src, GAsyncResult* result, gpointer arg) {
            auto *w = static_cast<Window*>(arg);
            if (!w) return;

            GError* error = NULL;
            auto value = webkit_web_view_evaluate_javascript_finish(
              WEBKIT_WEB_VIEW(src),
              result,
              &error
            );

            if (!value || error) return;

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
          "globalThis",
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

    opts.clientId = this->bridge->id;
    String preload = createPreload(opts);

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

    // ALWAYS on or off
    webkit_settings_set_enable_webgl(settings, true);
    webkit_settings_set_zoom_text_only(settings, false);
    webkit_settings_set_enable_mediasource(settings, true);
    webkit_settings_set_enable_encrypted_media(settings, true);
    webkit_settings_set_media_playback_allows_inline(settings, true);
    webkit_settings_set_enable_dns_prefetching(settings, true);

    // TODO(@jwerle); make configurable with '[permissions] allow_dialogs'
    webkit_settings_set_allow_modal_dialogs(
      settings,
      true
    );

    // TODO(@jwerle); make configurable with '[permissions] allow_media'
    webkit_settings_set_enable_media(settings, true);
    webkit_settings_set_enable_webaudio(settings, true);

    webkit_settings_set_enable_media_stream(
      settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_enable_media_capabilities(
      settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_enable_webrtc(
      settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_javascript_can_access_clipboard(
      settings,
      userConfig["permissions_allow_clipboard"] != "false"
    );

    webkit_settings_set_enable_fullscreen(
      settings,
      userConfig["permissions_allow_fullscreen"] != "false"
    );

    webkit_settings_set_enable_html5_local_storage(
      settings,
      userConfig["permissions_allow_data_access"] != "false"
    );

    webkit_settings_set_enable_html5_database(
      settings,
      userConfig["permissions_allow_data_access"] != "false"
    );

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

          if (geometry.width > 0) {
            width = geometry.width;
          }

          if (geometry.height > 0) {
            height = geometry.height;
          }

          break;
        }
      }

      g_list_free(list);
    }

    if (!height || !width) {
      auto geometry = GdkRectangle {};
      auto display = gdk_display_get_default();
      auto monitor = gdk_display_get_primary_monitor(display);

      if (monitor) {
        gdk_monitor_get_workarea(monitor, &geometry);
      }

      if (geometry.width > 0) {
        width = geometry.width;
      }

      if (geometry.height > 0) {
        height = geometry.height;
      }
    }

    if (!height) {
      height = (int) DEFAULT_MONITOR_HEIGHT;
    }

    if (!width) {
      width = (int) DEFAULT_MONITOR_WIDTH;
    }

    return ScreenSize { height, width };
  }

  void Window::eval (const String& source) {
    auto webview = this->webview;
    this->app.dispatch([=, this] {
      webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(this->webview),
        String(source).c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
      );
    });
  }

  void Window::show () {
    gtk_widget_realize(this->window);

    this->index = this->opts.index;
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
    // FIXME(@jwerle): this is deprecated
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

  void Window::maximize () {
    gtk_window_maximize(GTK_WINDOW(window));
  }

  void Window::minimize () {
    gtk_window_iconify(GTK_WINDOW(window));
  }

  void Window::restore () {
    gtk_window_deiconify(GTK_WINDOW(window));
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

  void Window::setTrayMenu (const String& seq, const String& value) {
    this->setMenu(seq, value, true);
  }

  void Window::setSystemMenu (const String& seq, const String& value) {
    this->setMenu(seq, value, false);
  }

  void Window::setMenu (const String& seq, const String& source, const bool& isTrayMenu) {
    if (source.empty()) {
      return;
    }

    auto menuSource = replace(SSC::String(source), "%%", "\n"); // copy and deserialize

    auto clear = [this](GtkWidget* menu) {
      GList *iter;
      GList *children = gtk_container_get_children(GTK_CONTAINER(menu));

      for (iter = children; iter != nullptr; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
      }

      g_list_free(children);
      return menu;
    };

    if (isTrayMenu) {
      menutray = menutray == nullptr ? gtk_menu_new() : clear(menutray);
    } else {
      menubar = menubar == nullptr ? gtk_menu_bar_new() : clear(menubar);
    }

    auto menus = split(menuSource, ';');

    for (auto m : menus) {
      auto menuSource = split(m, '\n');
      if (menuSource.size() == 0) continue;
      auto line = trim(menuSource[0]);
      if (line.empty()) continue;
      auto menuParts = split(line, ':');
      auto menuTitle = menuParts[0];
      // if this is a tray menu, append directly to the tray instead of a submenu.
      auto *ctx = isTrayMenu ? menutray : gtk_menu_new();
      GtkWidget *menuItem = gtk_menu_item_new_with_label(menuTitle.c_str());

      if (isTrayMenu && menuSource.size() == 1) {
        if (menuParts.size() > 1) {
          gtk_widget_set_name(menuItem, trim(menuParts[1]).c_str());
        }

        g_signal_connect(
          G_OBJECT(menuItem),
          "activate",
          G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
            auto w = static_cast<Window*>(arg);
            auto title = gtk_menu_item_get_label(GTK_MENU_ITEM(t));
            auto parent = gtk_widget_get_name(t);
            w->eval(getResolveMenuSelectionJavaScript("0", title, parent, "tray"));
          }),
          this
        );
      }

      for (int i = 1; i < menuSource.size(); i++) {
        auto line = trim(menuSource[i]);
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
              auto modifier = trim(accelerator[1]);
              // normalize modifier to lower case
              std::transform(
                modifier.begin(),
                modifier.end(),
                modifier.begin(),
                [](auto ch) { return std::tolower(ch); }
              );
              key = trim(parts[1]) == "_" ? "" : trim(accelerator[0]);

              GdkModifierType mask = (GdkModifierType)(0);
              bool isShift = String("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;

              if (accelerator.size() > 1) {
                if (modifier.find("meta") != -1 || modifier.find("super") != -1) {
                  mask = (GdkModifierType)(mask | GDK_META_MASK);
                }

                if (modifier.find("commandorcontrol") != -1) {
                  mask = (GdkModifierType)(mask | GDK_CONTROL_MASK);
                } else if (modifier.find("control") != -1) {
                  mask = (GdkModifierType)(mask | GDK_CONTROL_MASK);
                }

                if (modifier.find("alt") != -1) {
                  mask = (GdkModifierType)(mask | GDK_MOD1_MASK);
                }
              }

              if (isShift || modifier.find("shift") != -1) {
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

          if (isTrayMenu) {
            g_signal_connect(
              G_OBJECT(item),
              "activate",
              G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
                auto w = static_cast<Window*>(arg);
                auto title = gtk_menu_item_get_label(GTK_MENU_ITEM(t));
                auto parent = gtk_widget_get_name(t);

                w->eval(getResolveMenuSelectionJavaScript("0", title, parent, "tray"));
              }),
              this
            );
          } else {
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

                w->eval(getResolveMenuSelectionJavaScript("0", title, parent, "system"));
              }),
              this
            );
          }
        }

        gtk_widget_set_name(item, menuTitle.c_str());
        gtk_menu_shell_append(GTK_MENU_SHELL(ctx), item);
      }

      if (isTrayMenu) {
        gtk_menu_shell_append(GTK_MENU_SHELL(menutray), menuItem);
      } else {
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), ctx);
        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuItem);
      }
    }

    if (isTrayMenu) {
      static auto userConfig = SSC::getUserConfig();
      static auto app = App::instance();
      GtkStatusIcon *trayIcon;
      auto cwd = app->getcwd();
      auto trayIconPath = String("application_tray_icon");

      if (fs::exists(fs::path(cwd) / (trayIconPath + ".png"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".png")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpeg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpeg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".ico"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".ico")).string();
      } else {
        trayIconPath = "";
      }

      if (trayIconPath.size() > 0) {
        trayIcon = gtk_status_icon_new_from_file(trayIconPath.c_str());
      } else {
        trayIcon = gtk_status_icon_new_from_icon_name("utilities-terminal");
      }

      if (userConfig.count("tray_tooltip") > 0) {
        gtk_status_icon_set_tooltip_text(trayIcon, userConfig["tray_tooltip"].c_str());
      }

      g_signal_connect(
        trayIcon,
        "activate",
        G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
          auto w = static_cast<Window*>(arg);
          gtk_menu_popup_at_pointer(GTK_MENU(w->menutray), NULL);
          w->bridge->router.emit("tray", "true");
        }),
        this
      );
      gtk_widget_show_all(menutray);
    } else {
      gtk_box_pack_start(GTK_BOX(vbox), menubar, false, false, 0);
      gtk_widget_show_all(menubar);
    }

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
      this->eval(getResolveMenuSelectionJavaScript(seq, "", "contextMenu", "context"));
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

          window->eval(getResolveMenuSelectionJavaScript(seq, title, "contextMenu", "context"));
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
}
