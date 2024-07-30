#include <fstream>
#include <sys/utsname.h>

#include "../app/app.hh"
#include "window.hh"

static GtkTargetEntry droppableTypes[] = {
  { (char*) "text/uri-list", 0, 0 }
};

#define DEFAULT_MONITOR_WIDTH 720
#define DEFAULT_MONITOR_HEIGHT 364

namespace SSC {
  static void initializeWebContextFromWindow (Window* window) {
    static Atomic<bool> isInitialized = false;

    if (isInitialized) {
      return;
    }

    auto webContext = webkit_web_context_get_default();

    // mounts are configured for all contexts just once
    window->bridge.configureNavigatorMounts();

    g_signal_connect(
      G_OBJECT(webContext),
      "initialize-notification-permissions",
      G_CALLBACK(+[](
        WebKitWebContext* webContext,
        gpointer userData
      ) {
        static const auto app = App::sharedApplication();
        static const auto bundleIdentifier = app->userConfig["meta_bundle_identifier"];
        static const auto areNotificationsAllowed = (
          !app->userConfig.contains("permissions_allow_notifications") ||
          app->userConfig.at("permissions_allow_notifications") != "false"
        );

        const auto uri = "socket://" + bundleIdentifier;
        const auto origin = webkit_security_origin_new_for_uri(uri.c_str());

        GList* allowed = nullptr;
        GList* disallowed = nullptr;

        webkit_security_origin_ref(origin);

        if (origin) {
          if (areNotificationsAllowed) {
            disallowed = g_list_append(disallowed, (gpointer) origin);
          } else {
            allowed = g_list_append(allowed, (gpointer) origin);
          }

          if (allowed || disallowed) {
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
      nullptr
    );

    webkit_web_context_set_sandbox_enabled(webContext, true);

    auto extensionsPath = FileResource::getResourcePath(Path("lib/extensions"));
    webkit_web_context_set_web_extensions_directory(
      webContext,
      extensionsPath.c_str()
    );

    auto cwd = getcwd();
    auto bytes = socket_runtime_init_get_user_config_bytes();
    auto size = socket_runtime_init_get_user_config_bytes_size();
    static auto data = String(reinterpret_cast<const char*>(bytes), size);
    data += "[web-process-extension]\n";
    data += "cwd = " + cwd + "\n";
    data += "host = " + getDevHost() + "\n";
    data += "port = " + std::to_string(getDevPort()) + "\n";

    webkit_web_context_set_web_extensions_initialization_user_data(
      webContext,
      g_variant_new_from_data(
        G_VARIANT_TYPE("ay"), // an array of "bytes"
        data.c_str(),
        data.size(),
        true,
        nullptr,
        nullptr
      )
    );

    isInitialized = true;
  }

  Window::Window (SharedPointer<Core> core, const Window::Options& options)
    : core(core),
      options(options),
      bridge(core, IPC::Bridge::Options {
        options.userConfig,
        options.as<IPC::Preload::Options>()
      }),
      hotkey(this),
      dialog(this)
  {
    Env::set("GTK_OVERLAY_SCROLLING", "1");

    auto userConfig = options.userConfig;
    auto webContext = webkit_web_context_get_default();

    if (options.index == 0) {
      initializeWebContextFromWindow(this);
    }

    this->settings = webkit_settings_new();
    // TODO(@jwerle); make configurable with '[permissions] allow_media'
    webkit_settings_set_zoom_text_only(this->settings, false);
    webkit_settings_set_media_playback_allows_inline(this->settings, true);
    // TODO(@jwerle); make configurable with '[permissions] allow_dialogs'
    webkit_settings_set_allow_modal_dialogs(this->settings, true);
    webkit_settings_set_hardware_acceleration_policy(
      this->settings,
      userConfig["permissions_hardware_acceleration_disabled"] == "true"
        ? WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER
        : WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
    );

    webkit_settings_set_enable_webgl(this->settings, true);
    webkit_settings_set_enable_media(this->settings, true);
    webkit_settings_set_enable_webaudio(this->settings, true);
    webkit_settings_set_enable_mediasource(this->settings, true);
    webkit_settings_set_enable_encrypted_media(this->settings, true);
    webkit_settings_set_enable_dns_prefetching(this->settings, true);
    webkit_settings_set_enable_smooth_scrolling(this->settings, true);
    webkit_settings_set_enable_developer_extras(this->settings, options.debug);
    webkit_settings_set_enable_back_forward_navigation_gestures(this->settings, true);
    webkit_settings_set_media_content_types_requiring_hardware_support(
      this->settings,
      nullptr
    );

    auto userAgent = String(webkit_settings_get_user_agent(settings));

    webkit_settings_set_user_agent(
      settings,
      (userAgent + " " + "SocketRuntime/" + SSC::VERSION_STRING).c_str()
    );

    webkit_settings_set_enable_media_stream(
      this->settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_enable_media_capabilities(
      this->settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_enable_webrtc(
      this->settings,
      userConfig["permissions_allow_user_media"] != "false"
    );

    webkit_settings_set_javascript_can_access_clipboard(
      this->settings,
      userConfig["permissions_allow_clipboard"] != "false"
    );

    webkit_settings_set_enable_fullscreen(
      this->settings,
      userConfig["permissions_allow_fullscreen"] != "false"
    );

    webkit_settings_set_enable_html5_local_storage(
      this->settings,
      userConfig["permissions_allow_data_access"] != "false"
    );

    webkit_settings_set_enable_html5_database(
      this->settings,
      userConfig["permissions_allow_data_access"] != "false"
    );

    this->accelGroup = gtk_accel_group_new();
    this->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    auto cookieManager = webkit_web_context_get_cookie_manager(webContext);
    webkit_cookie_manager_set_accept_policy(cookieManager, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    this->userContentManager = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(this->userContentManager, "external");

    auto preloadUserScriptSource = IPC::Preload::compile({
      .features = IPC::Preload::Options::Features {
        .useGlobalCommonJS = false,
        .useGlobalNodeJS = false,
        .useTestScript = false,
        .useHTMLMarkup = false,
        .useESM = false,
        .useGlobalArgs = true
      },
      .client = UniqueClient {
        .id = this->bridge.client.id,
        .index = this->bridge.client.index
      },
      .index = options.index,
      .conduit = this->core->conduit.port,
      .userScript = options.userScript
    });

    auto preloadUserScript = webkit_user_script_new(
      preloadUserScriptSource.str().c_str(),
      WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
      WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
      nullptr,
      nullptr
    );

    webkit_user_content_manager_add_script(
      this->userContentManager,
      preloadUserScript
    );

    this->policies = webkit_website_policies_new_with_policies(
      "autoplay", userConfig["permission_allow_autoplay"] != "false"
        ? WEBKIT_AUTOPLAY_ALLOW
        : WEBKIT_AUTOPLAY_DENY,
      nullptr
    );

    this->webview = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
      "user-content-manager", this->userContentManager,
      "website-policies", this->policies,
      "web-context", webContext,
      "settings", this->settings,
      nullptr
    ));

    gtk_widget_set_can_focus(GTK_WIDGET(this->webview), true);

    this->index = this->options.index;
    this->dragStart = {0,0};
    this->shouldDrag = false;
    this->contextMenu = nullptr;
    this->contextMenuID = 0;

    this->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    this->bridge.navigateFunction = [this] (const auto url) {
      this->navigate(url);
    };

    this->bridge.evaluateJavaScriptFunction = [this] (const auto source) {
      this->eval(source);
    };

    this->bridge.client.preload = IPC::Preload::compile({
      .client = UniqueClient {
        .id = this->bridge.client.id,
        .index = this->bridge.client.index
      },
      .index = options.index,
      .conduit = this->core->conduit.port,
      .userScript = options.userScript
    });

    gtk_box_pack_end(GTK_BOX(this->vbox), GTK_WIDGET(this->webview), true, true, 0);

    gtk_container_add(GTK_CONTAINER(this->window), this->vbox);

    gtk_widget_add_events(this->window, GDK_ALL_EVENTS_MASK);
    gtk_widget_grab_focus(GTK_WIDGET(this->webview));
    gtk_widget_realize(GTK_WIDGET(this->window));

    if (options.resizable) {
      gtk_window_set_default_size(GTK_WINDOW(this->window), options.width, options.height);
    } else {
      gtk_widget_set_size_request(this->window, options.width, options.height);
    }

    gtk_window_set_decorated(GTK_WINDOW(this->window), options.frameless == false);
    gtk_window_set_resizable(GTK_WINDOW(this->window), options.resizable);
    gtk_window_set_position(GTK_WINDOW(this->window), GTK_WIN_POS_CENTER);
    gtk_widget_set_can_focus(GTK_WIDGET(this->window), true);

    GdkRGBA webviewBackground = {0.0, 0.0, 0.0, 0.0};
    bool hasDarkValue = this->options.backgroundColorDark.size();
    bool hasLightValue = this->options.backgroundColorLight.size();

    auto isKDEDarkMode = []() -> bool {
      static const auto paths = FileResource::getWellKnownPaths();
      static const auto kdeglobals = paths.home / ".config" / "kdeglobals";

      if (!FileResource::isFile(kdeglobals)) {
        return false;
      }

      auto file = FileResource(kdeglobals);

      if (!file.exists() || !file.hasAccess()) {
        return false;
      }

      const auto bytes = file.read();
      const auto lines = split(bytes, '\n');

      for (const auto& line : lines) {
        if (toLowerCase(line).find("dark") != String::npos) {
          return true;
        }
      }

      return false;
    };

    auto isGnomeDarkMode = [this]() -> bool {
      GtkStyleContext* context = gtk_widget_get_style_context(this->window);
      GdkRGBA background_color;
      // FIXME(@jwerle): this is deprecated
      gtk_style_context_get_background_color(context, GTK_STATE_FLAG_NORMAL, &background_color);

      bool is_dark_theme = (0.299*  background_color.red +
                            0.587*  background_color.green +
                            0.114*  background_color.blue) < 0.5;

      return FALSE;
    };

    if (hasDarkValue || hasLightValue) {
      GdkRGBA color = {0};

      const gchar* desktop_env = getenv("XDG_CURRENT_DESKTOP");

      if (desktop_env != NULL && g_str_has_prefix(desktop_env, "GNOME")) {
        this->isDarkMode = isGnomeDarkMode();
      } else {
        this->isDarkMode = isKDEDarkMode();
      }

      if (this->isDarkMode && hasDarkValue) {
        gdk_rgba_parse(&color, this->options.backgroundColorDark.c_str());
      } else if (hasLightValue) {
        gdk_rgba_parse(&color, this->options.backgroundColorLight.c_str());
      }

      // FIXME(@jwerle): this is deprecated
      gtk_widget_override_background_color(this->window, GTK_STATE_FLAG_NORMAL, &color);
    }

    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), &webviewBackground);

    this->hotkey.init();
    this->bridge.init();
    this->bridge.configureSchemeHandlers({
      .webview = settings
    });

    this->bridge.configureWebView(this->webview);

    g_signal_connect(
      this->userContentManager,
      "script-message-received::external",
      G_CALLBACK(+[](
        WebKitUserContentManager* userContentManager,
        WebKitJavascriptResult* result,
        gpointer ptr
      ) {
        auto window = static_cast<Window*>(ptr);
        auto value = webkit_javascript_result_get_js_value(result);
        auto valueString = jsc_value_to_string(value);
        auto str = String(valueString);

        if (!window->bridge.route(str, nullptr, 0)) {
          if (window->onMessage != nullptr) {
            window->onMessage(str);
          }
        }

        g_free(valueString);
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "show-notification",
      G_CALLBACK(+[](
        WebKitWebView* webview,
        WebKitNotification* notification,
        gpointer userData
      ) -> bool {
        const auto app = App::sharedApplication();
        const auto window = app->windowManager.getWindowForWebView(webview);

        if (window == nullptr) {
          return true;
        }

        auto userConfig = window->bridge.userConfig;
        if (userConfig["permissions_allow_notifications"] == "false") {
          return true;
        }

        return false;
      }),
      this
    );

    // handle `navigator.permissions.query()`
    g_signal_connect(
      G_OBJECT(this->webview),
      "query-permission-state",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPermissionStateQuery* query,
        gpointer user_data
      ) -> bool {
        const auto app = App::sharedApplication();
        const auto window = app->windowManager.getWindowForWebView(webview);
        const auto name = String(webkit_permission_state_query_get_name(query));

        if (name == "geolocation") {
          webkit_permission_state_query_finish(
            query,
            window->bridge.userConfig["permissions_allow_geolocation"] == "false"
              ? WEBKIT_PERMISSION_STATE_DENIED
              : WEBKIT_PERMISSION_STATE_GRANTED
          );
        } else if (name == "notifications") {
          webkit_permission_state_query_finish(
            query,
            window->bridge.userConfig["permissions_allow_notifications"] == "false"
              ? WEBKIT_PERMISSION_STATE_DENIED
              : WEBKIT_PERMISSION_STATE_GRANTED
          );
        } else {
          webkit_permission_state_query_finish(
            query,
            WEBKIT_PERMISSION_STATE_PROMPT
          );
        }
        return true;
      })),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "permission-request",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPermissionRequest* request,
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
          if (userConfig["permissions_allow_user_media"] != "false") {
            if (webkit_user_media_permission_is_for_audio_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
              name = "microphone";
              result = userConfig["permissions_allow_microphone"] != "false";
              description = "{{meta_title}} would like access to your microphone.";
            }

            if (webkit_user_media_permission_is_for_video_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
              name = "camera";
              result = userConfig["permissions_allow_camera"] != "false";
              description = "{{meta_title}} would like access to your camera.";
            }
          }
        } else if (WEBKIT_IS_WEBSITE_DATA_ACCESS_PERMISSION_REQUEST(request)) {
          name = "storage-access";
          result = userConfig["permissions_allow_data_access"] != "false";
          description = "{{meta_title}} would like access to local storage.";
        } else if (WEBKIT_IS_DEVICE_INFO_PERMISSION_REQUEST(request)) {
          result = userConfig["permissions_allow_device_info"] != "false";
          description = "{{meta_title}} would like access to your device information.";
          if (result) {
            webkit_permission_request_allow(request);
            return result;
          }
        } else if (WEBKIT_IS_MEDIA_KEY_SYSTEM_PERMISSION_REQUEST(request)) {
          result = userConfig["permissions_allow_media_key_system"] != "false";
          description = "{{meta_title}} would like access to your media key system.";
        }

        if (result) {
          auto title = userConfig["meta_title"];
          GtkWidget* dialog = gtk_message_dialog_new(
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
          JSON::Object json = JSON::Object::Entries {
            {"name", name},
            {"state", result ? "granted" : "denied"}
          };
          // TODO(@heapwolf): properly return this data
          // TODO(@jwerle): maybe this could be dispatched to webview
        }

        return result;
      })),
      this
    );

    // Calling gtk_drag_source_set interferes with text selection
    /* gtk_drag_source_set(
      webview,
      (GdkModifierType)(GDK_BUTTON1_MASK | GDK_BUTTON2_MASK),
      droppableTypes,
      G_N_ELEMENTS(droppableTypes),
      GDK_ACTION_COPY
    );

    gtk_drag_dest_set(
      webview,
      GTK_DEST_DEFAULT_ALL,
      droppableTypes,
      1,
      GDK_ACTION_MOVE
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "button-release-event",
      G_CALLBACK(+[](GtkWidget* wv, GdkEventButton* event, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        w->shouldDrag = false;
        w->dragStart.x = 0;
        w->dragStart.y = 0;
        w->dragging.x = 0;
        w->dragging.y = 0;
        return FALSE;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "button-press-event",
      G_CALLBACK(+[](GtkWidget* wv, GdkEventButton* event, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        w->shouldDrag = false;

        if (event->button == GDK_BUTTON_PRIMARY) {
          auto win = GDK_WINDOW(gtk_widget_get_window(w->window));
          gint initialX;
          gint initialY;

          gdk_window_get_position(win, &initialX, &initialY);

          w->dragStart.x = initialX;
          w->dragStart.y = initialY;

          w->dragging.x = event->x_root;
          w->dragging.y = event->y_root;

          GdkDevice* device;

          gint x = event->x;
          gint y = event->y;
          String sx = std::to_string(x);
          String sy = std::to_string(y);

          String js(
            "(() => {                                                                      "
            "  const v = '--app-region';                                                   "
            "  let el = document.elementFromPoint(" + sx + "," + sy + ");                  "
            "                                                                              "
            "  while (el) {                                                                "
            "    if (getComputedStyle(el).getPropertyValue(v) == 'drag') return 'movable'; "
            "    el = el.parentElement;                                                    "
            "  }                                                                           "
            "  return ''                                                                   "
            "})()                                                                          "
          );

          webkit_web_view_evaluate_javascript(
            WEBKIT_WEB_VIEW(wv),
            js.c_str(),
            -1,
            nullptr,
            nullptr,
            nullptr,
            [](GObject* src, GAsyncResult* result, gpointer arg) {
              auto* w = static_cast<Window*>(arg);
              if (!w) return;

              GError* error = NULL;
              auto value = webkit_web_view_evaluate_javascript_finish(
                WEBKIT_WEB_VIEW(w->webview),
                result,
                &error
              );

              if (error) return;
              if (!value) return;
              if (!jsc_value_is_string(value)) return;

              auto match = std::string(jsc_value_to_string(value));
              w->shouldDrag = match == "movable";
              return;
            },
            w
          );
        }

        return FALSE;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "focus",
      G_CALLBACK(+[](
        GtkWidget* wv,
        GtkDirectionType direction,
        gpointer arg)
      {
        auto* w = static_cast<Window*>(arg);
        if (!w) return;
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "drag-data-get",
      G_CALLBACK(+[](
        GtkWidget* wv,
        GdkDragContext* context,
        GtkSelectionData* data,
        guint info,
        guint time,
        gpointer arg)
      {
        auto* w = static_cast<Window*>(arg);
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
      G_OBJECT(this->webview),
      "motion-notify-event",
      G_CALLBACK(+[](
        GtkWidget* wv,
        GdkEventMotion* event,
        gpointer arg)
      {
        auto* w = static_cast<Window*>(arg);
        if (!w) return FALSE;

        if (w->shouldDrag && event->state & GDK_BUTTON1_MASK) {
          auto win = GDK_WINDOW(gtk_widget_get_window(w->window));
          gint x;
          gint y;

          GdkRectangle frame_extents;
          gdk_window_get_frame_extents(win, &frame_extents);

          GtkAllocation allocation;
          gtk_widget_get_allocation(wv, &allocation);

          gint menubarHeight = 0;

          if (w->menubar) {
            GtkAllocation allocationMenubar;
            gtk_widget_get_allocation(w->menubar, &allocationMenubar);
            menubarHeight = allocationMenubar.height;
          }

          int offsetWidth = (frame_extents.width - allocation.width) / 2;
          int offsetHeight = (frame_extents.height - allocation.height) - offsetWidth - menubarHeight;

          gdk_window_get_position(win, &x, &y);

          gint offset_x = event->x_root - w->dragging.x;
          gint offset_y = event->y_root - w->dragging.y;

          gint newX = x + offset_x;
          gint newY = y + offset_y;

          gdk_window_move(win, newX - offsetWidth, newY - offsetHeight);

          w->dragging.x = event->x_root;
          w->dragging.y = event->y_root;
        }

        return FALSE;

        //
        // TODO(@heapwolf): refactor legacy drag and drop stuff
        //
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
      G_OBJECT(this->webview),
      "drag-end",
      G_CALLBACK(+[](GtkWidget* wv, GdkDragContext* context, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        if (!w) return;

        // w->isDragInvokedInsideWindow = false;
        // w->draggablePayload.clear();
        w->eval(getEmitToRenderProcessJavaScript("dragend", "{}"));
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(this->webview),
      "drag-data-received",
      G_CALLBACK(+[](
        GtkWidget* wv,
        GdkDragContext* context,
        gint x,
        gint y,
        GtkSelectionData* data,
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
      G_OBJECT(this->webview),
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
    */

    g_signal_connect(
      G_OBJECT(this->window),
      "destroy",
      G_CALLBACK((+[](GtkWidget* object, gpointer arg) {
        auto app = App::sharedApplication();
        if (app == nullptr || app->shouldExit) {
          return FALSE;
        }

        auto w = reinterpret_cast<Window*>(arg);
        int index = w != nullptr ? w->index : -1;

        for (auto& window : app->windowManager.windows) {
          if (window == nullptr) {
            continue;
          }

          if (window->window == object) {
            index = window->index;
            if (window->webview) {
              window->webview = g_object_ref(window->webview);
            }
            window->window = nullptr;
            window->vbox = nullptr;
            break;
          }
        }

        if (index >= 0) {
          for (auto window : app->windowManager.windows) {
            if (window == nullptr || window->index == index) {
              continue;
            }

            JSON::Object json = JSON::Object::Entries {
              {"data", index}
            };

            window->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));
          }

          app->windowManager.destroyWindow(index);
        }
        return FALSE;
      })),
      this
    );

    g_signal_connect(
      G_OBJECT(this->window),
      "size-allocate", // https://docs.gtk.org/gtk3/method.Window.get_size.html
      G_CALLBACK(+[](GtkWidget* widget,GtkAllocation* allocation, gpointer arg) {
        auto* w = static_cast<Window*>(arg);
        gtk_window_get_size(GTK_WINDOW(widget), &w->size.width, &w->size.height);
      }),
      this
    );

    if (this->options.aspectRatio.size() > 0) {
      g_signal_connect(
        window,
        "configure-event",
        G_CALLBACK(+[](GtkWidget* widget, GdkEventConfigure* event, gpointer ptr) {
          auto w = static_cast<Window*>(ptr);
          if (!w) return FALSE;

          // TODO(@heapwolf): make the parsed aspectRatio properties so it doesnt need to be recalculated.
          auto parts = split(w->options.aspectRatio, ':');
          float aspectWidth = 0;
          float aspectHeight = 0;

          try {
            aspectWidth = std::stof(trim(parts[0]));
            aspectHeight = std::stof(trim(parts[1]));
          } catch (...) {
            debug("invalid aspect ratio");
            return FALSE;
          }

          if (aspectWidth > 0 && aspectHeight > 0) {
            GdkGeometry geom;
            geom.min_aspect = aspectWidth / aspectHeight;
            geom.max_aspect = geom.min_aspect;
            gtk_window_set_geometry_hints(GTK_WINDOW(widget), widget, &geom, GdkWindowHints(GDK_HINT_ASPECT));
          }

          return FALSE;
        }),
        this
      );

      // gtk_window_set_aspect_ratio(GTK_WINDOW(window), aspectRatio, TRUE);
    }
  }

  Window::~Window () {
    if (this->policies) {
      g_object_unref(this->policies);
      this->policies = nullptr;
    }

    if (this->settings) {
      g_object_unref(this->settings);
      this->settings = nullptr;
    }

    if (this->userContentManager) {
      g_object_unref(this->userContentManager);
      this->userContentManager = nullptr;
    }

    if (this->accelGroup) {
      g_object_unref(this->accelGroup);
      this->accelGroup = nullptr;
    }

    if (this->webview) {
      gtk_widget_set_can_focus(GTK_WIDGET(this->webview), false);
      this->webview = nullptr;
    }

    if (this->vbox) {
      gtk_container_remove(GTK_CONTAINER(this->window), this->vbox);
      this->vbox = nullptr;
    }

    if (this->window) {
      auto window = this->window;
      this->window = nullptr;
      g_object_unref(window);
    }
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

    return ScreenSize { width, height };
  }

  void Window::eval (const String& source, const EvalCallback& callback) {
    auto app = App::sharedApplication();
    app->dispatch([=, this] () {
      if (this->webview) {
        webkit_web_view_evaluate_javascript(
          this->webview,
          source.c_str(),
          source.size(),
          nullptr, // world name
          nullptr, // source URI
          nullptr, // cancellable
          +[]( // callback
            GObject* object,
            GAsyncResult* result,
            gpointer userData
          ) {
            const auto callback = reinterpret_cast<const EvalCallback*>(userData);
            if (callback == nullptr || *callback == nullptr) {
              auto value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, nullptr);
              return;
            }

            GError* error = nullptr;
            auto value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error);

            if (!value) {
              if (error != nullptr) {
                (*callback)(JSON::Error(error->message));
                g_error_free(error);
              } else {
                (*callback)(JSON::Error("An unknown error occurred"));
              }
            } else if (jsc_value_is_string(value)) {
              const auto context = jsc_value_get_context(value);
              const auto exception = jsc_context_get_exception(context);
              const auto stringValue = jsc_value_to_string(value);

              if (exception) {
                const auto message = jsc_exception_get_message(exception);
                (*callback)(JSON::Error(message));
              } else {
                (*callback)(stringValue);
              }

              g_free(stringValue);
            } else if (jsc_value_is_boolean(value)) {
              const auto context = jsc_value_get_context(value);
              const auto exception = jsc_context_get_exception(context);
              const auto booleanValue = jsc_value_to_boolean(value);

              if (exception) {
                const auto message = jsc_exception_get_message(exception);
                (*callback)(JSON::Error(message));
              } else {
                (*callback)(booleanValue);
              }
            } else if (jsc_value_is_null(value)) {
              const auto context = jsc_value_get_context(value);
              const auto exception = jsc_context_get_exception(context);

              if (exception) {
                const auto message = jsc_exception_get_message(exception);
                (*callback)(JSON::Error(message));
              } else {
                (*callback)(nullptr);
              }
            } else if (jsc_value_is_number(value)) {
              const auto context = jsc_value_get_context(value);
              const auto exception = jsc_context_get_exception(context);
              const auto numberValue = jsc_value_to_double(value);

              if (exception) {
                const auto message = jsc_exception_get_message(exception);
                (*callback)(JSON::Error(message));
              } else {
                (*callback)(numberValue);
              }
            } else if (jsc_value_is_undefined(value)) {
              const auto context = jsc_value_get_context(value);
              const auto exception = jsc_context_get_exception(context);

              if (exception) {
                const auto message = jsc_exception_get_message(exception);
                (*callback)(JSON::Error(message));
              } else {
                (*callback)(nullptr);
              }
            }

            if (value) {
              //webkit_javascript_result_unref(reinterpret_cast<WebKitJavascriptResult*>(value));
            }

            delete callback;
          },
          callback == nullptr
            ? nullptr
            : new EvalCallback(std::move(callback))
        );
      }
    });
  }

  void Window::show () {
    auto app = App::sharedApplication();
    app->dispatch([=, this] () {
      if (this->window != nullptr) {
        gtk_widget_realize(this->window);

        this->index = this->options.index;
        if (this->options.headless == false) {
          gtk_widget_show_all(this->window);
          gtk_window_present(GTK_WINDOW(this->window));
        }
      }
    });
  }

  void Window::hide () {
    gtk_widget_realize(this->window);
    gtk_widget_hide(this->window);
    this->eval(getEmitToRenderProcessJavaScript("window-hidden", "{}"));
  }

  void Window::setBackgroundColor (const String& rgba) {
    const auto parts = split(trim(replace(replace(rgba, "rgba(", ""), ")", "")), ',');
    int r = 0, g = 0, b = 0;
    float a = 1.0;

    if (parts.size() == 4) {
      try { r = std::stoi(trim(parts[0])); }
      catch (...) {}

      try { g = std::stoi(trim(parts[1])); }
      catch (...) {}

      try { b = std::stoi(trim(parts[2])); }
      catch (...) {}

      try { a = std::stof(trim(parts[3])); }
      catch (...) {}

      return this->setBackgroundColor(r, g, b, a);
    }
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    GdkRGBA color;
    color.red = r / 255.0;
    color.green = g / 255.0;
    color.blue = b / 255.0;
    color.alpha = a;

    if (this->window) {
      gtk_widget_realize(this->window);
      // FIXME(@jwerle): this is deprecated
      gtk_widget_override_background_color(
        this->window, GTK_STATE_FLAG_NORMAL, &color
      );
    }
  }

  String Window::getBackgroundColor () {
    GtkStyleContext* context = gtk_widget_get_style_context(this->window);

    GdkRGBA color;
    // FIXME(@jwerle): this is deprecated
    gtk_style_context_get_background_color(context, gtk_widget_get_state_flags(this->window), &color);

    char string[100];

    snprintf(
      string,
      sizeof(string),
      "rgba(%d, %d, %d, %f)",
      (int) (255 * color.red),
      (int) (255 * color.green),
      (int) (255 * color.blue),
      color.alpha
    );

    return string;
  }

  void Window::showInspector () {
    if (this->webview) {
      const auto inspector = webkit_web_view_get_inspector(this->webview);
      if (inspector) {
        webkit_web_inspector_show(inspector);
      }
    }
  }

  void Window::exit (int code) {
    const auto callback = this->onExit;
    this->onExit = nullptr;
    if (callback != nullptr) {
      callback(code);
    }
  }

  void Window::kill () {}

  void Window::close (int _) {
    if (this->window) {
      g_object_ref(this->window);
      gtk_window_close(GTK_WINDOW(this->window));
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

  void Window::navigate (const String& url) {
    static auto app = App::sharedApplication();
    auto webview = this->webview;
    if (webview) {
      app->dispatch([=] () {
        webkit_web_view_load_uri(webview, url.c_str());
      });
    }
  }

  const String Window::getTitle () const {
    if (this->window != nullptr) {
      const auto title = gtk_window_get_title(GTK_WINDOW(this->window));
      if (title != nullptr) {
        return title;
      }
    }

    return "";
  }

  void Window::setTitle (const String& s) {
    gtk_widget_realize(window);
    gtk_window_set_title(GTK_WINDOW(window), s.c_str());
  }

  void Window::about () {
    GtkWidget* dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);

    GtkWidget* body = gtk_dialog_get_content_area(GTK_DIALOG(GTK_WINDOW(dialog)));
    GtkContainer* content = GTK_CONTAINER(body);

    String imgPath = "/usr/share/icons/hicolor/256x256/apps/" +
      this->bridge.userConfig["build_name"] +
      ".png";

    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(
      imgPath.c_str(),
      60,
      60,
      true,
      nullptr
    );

    GtkWidget* img = gtk_image_new_from_pixbuf(pixbuf);
    gtk_widget_set_margin_top(img, 20);
    gtk_widget_set_margin_bottom(img, 20);

    gtk_box_pack_start(GTK_BOX(content), img, false, false, 0);

    String title_value(this->bridge.userConfig["build_name"] + " v" + this->bridge.userConfig["meta_version"]);
    String version_value("Built with ssc v" + SSC::VERSION_FULL_STRING);

    GtkWidget* label_title = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_title), title_value.c_str());
    gtk_container_add(content, label_title);

    GtkWidget* label_op_version = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_op_version), version_value.c_str());
    gtk_container_add(content, label_op_version);

    GtkWidget* label_copyright = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_copyright), this->bridge.userConfig["meta_copyright"].c_str());
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

  Window::Size Window::getSize () {
    gtk_widget_get_size_request(
      this->window,
      &this->size.width,
      &this->size.height
    );

    return this->size;
  }

  const Window::Size Window::getSize () const {
    return this->size;
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

    this->size.width = width;
    this->size.height = height;
  }

  void Window::setPosition (float x, float y) {
    gtk_window_move(GTK_WINDOW(this->window), (int) x, (int) y);
    this->position.x = x;
    this->position.y = y;
  }

  void Window::setTrayMenu (const String& value) {
    this->setMenu(value, true);
  }

  void Window::setSystemMenu (const String& value) {
    this->setMenu(value, false);
  }

  void Window::setMenu (const String& menuSource, const bool& isTrayMenu) {
    if (menuSource.empty()) {
      return;
    }

    auto clear = [this](GtkWidget* menu) {
      GList* iter;
      GList* children = gtk_container_get_children(GTK_CONTAINER(menu));

      for (iter = children; iter != nullptr; iter = g_list_next(iter)) {
        if (iter && iter->data) {
          gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
      }

      g_list_free(children);
      return menu;
    };

    if (isTrayMenu) {
      menutray = menutray == nullptr ? gtk_menu_new() : clear(menutray);
    } else {
      menubar = menubar == nullptr ? gtk_menu_bar_new() : clear(menubar);
    }

    GtkStyleContext* context = gtk_widget_get_style_context(this->window);

    GdkRGBA color = {0.0, 0.0, 0.0, 0.0};
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(this->webview), &color);
    gtk_widget_override_background_color(menubar, GTK_STATE_FLAG_NORMAL, &color);

    auto menus = split(menuSource, ';');

    for (auto m : menus) {
      auto menuSource = split(m, '\n');
      if (menuSource.size() == 0) continue;
      auto line = trim(menuSource[0]);
      if (line.empty()) continue;
      auto menuParts = split(line, ':');
      auto menuTitle = menuParts[0];
      // if this is a tray menu, append directly to the tray instead of a submenu.
      auto* ctx = isTrayMenu ? menutray : gtk_menu_new();
      GtkWidget* menuItem = gtk_menu_item_new_with_label(menuTitle.c_str());
      gtk_widget_override_background_color(menuItem, GTK_STATE_FLAG_NORMAL, &color);

      if (isTrayMenu && menuSource.size() == 1) {
        if (menuParts.size() > 1) {
          gtk_widget_set_name(menuItem, trim(menuParts[1]).c_str());
        }

        g_signal_connect(
          G_OBJECT(menuItem),
          "activate",
          G_CALLBACK(+[](GtkWidget* t, gpointer arg) {
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

        GtkWidget* item;

        if (parts[0].find("---") != -1) {
          item = gtk_separator_menu_item_new();
        } else {
          item = gtk_menu_item_new_with_label(title.c_str());

          if (parts.size() > 1) {
            auto value = trim(parts[1]);
            key = value == "_" ? "" : value;

            if (key.size() > 0) {
              auto accelerator = split(parts[1], '+');
              if (accelerator.size() <= 1) {
                continue;
              }

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
              G_CALLBACK(+[](GtkWidget* t, gpointer arg) {
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
              G_CALLBACK(+[](GtkWidget* t, gpointer arg) {
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
        gtk_widget_override_background_color(menuItem, GTK_STATE_FLAG_NORMAL, &color);
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
      static auto app = App::sharedApplication();
      GtkStatusIcon* trayIcon;
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
        G_CALLBACK(+[](GtkWidget* t, gpointer arg) {
          auto w = static_cast<Window*>(arg);
          gtk_menu_popup_at_pointer(GTK_MENU(w->menutray), NULL);
          w->bridge.emit("tray", true);
        }),
        this
      );
      gtk_widget_show_all(menutray);
    } else {
      gtk_box_pack_start(GTK_BOX(this->vbox), menubar, false, false, 0);
      gtk_widget_show_all(menubar);
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // @TODO(): provide impl
  }

  void Window::closeContextMenu () {
    if (this->contextMenuID > 0) {
      const auto seq = std::to_string(this->contextMenuID);
      this->contextMenuID = 0;
      closeContextMenu(seq);
    }
  }

  void Window::closeContextMenu (const String& seq) {
    if (contextMenu != nullptr) {
      auto ptr = contextMenu;
      contextMenu = nullptr;
      closeContextMenu(ptr, seq);
    }
  }

  void Window::closeContextMenu (
    GtkWidget* contextMenu,
    const String& seq
  ) {
    if (contextMenu != nullptr) {
      gtk_menu_popdown((GtkMenu* ) contextMenu);
      gtk_widget_destroy(contextMenu);
      this->eval(getResolveMenuSelectionJavaScript(seq, "", "contextMenu", "context"));
    }
  }

  void Window::setContextMenu (
    const String& seq,
    const String& menuSource
  ) {
    closeContextMenu();
    if (menuSource.empty()) return void(0);

    // members
    this->contextMenu = gtk_menu_new();

    try {
      this->contextMenuID = std::stoi(seq);
    } catch (...) {
      this->contextMenuID = 0;
    }

    auto menuItems = split(menuSource, '\n');

    for (auto itemData : menuItems) {
      if (trim(itemData).size() == 0) {
        continue;
      }

      if (itemData.find("---") != -1) {
        auto* item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(this->contextMenu), item);
        continue;
      }

      auto pair = split(itemData, ':');
      auto meta = String(seq + ";" + itemData);
      auto* item = gtk_menu_item_new_with_label(pair[0].c_str());

      g_signal_connect(
        G_OBJECT(item),
        "activate",
        G_CALLBACK(+[](GtkWidget* t, gpointer arg) {
          auto window = static_cast<Window*>(arg);
          if (!window) return;

          auto meta = gtk_widget_get_name(t);
          auto pair = split(meta, ';');
          auto seq = pair[0];
          auto items = split(pair[1], ":");

          if (items.size() != 2) return;
          window->eval(getResolveMenuSelectionJavaScript(seq, trim(items[0]), trim(items[1]), "context"));
        }),
        this
      );

      gtk_widget_set_name(item, meta.c_str());
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(this->contextMenu), item);
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

    gtk_widget_add_events(contextMenu, GDK_ALL_EVENTS_MASK);
    gtk_widget_set_can_focus(contextMenu, true);
    gtk_widget_show_all(contextMenu);
    gtk_widget_grab_focus(contextMenu);

    gtk_menu_popup_at_rect(
      GTK_MENU(contextMenu),
      win,
      &rect,
      GDK_GRAVITY_SOUTH_WEST,
      GDK_GRAVITY_NORTH_WEST,
      event
    );
  }

  void Window::handleApplicationURL (const String& url) {
    JSON::Object json = JSON::Object::Entries {{
      "url", url
    }};

    if (this->index == 0 && this->window && this->webview) {
      gtk_widget_show_all(GTK_WIDGET(this->window));
      gtk_widget_grab_focus(GTK_WIDGET(this->webview));
      gtk_widget_grab_focus(GTK_WIDGET(this->window));
      gtk_window_activate_focus(GTK_WINDOW(this->window));
      gtk_window_present(GTK_WINDOW(this->window));
    }

    this->bridge.emit("applicationurl", json.str());
  }
}
