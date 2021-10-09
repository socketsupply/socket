#include "common.hh"

#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace Opkit {

  class App : public IApp {

    public:
      App(int);

      static std::atomic<bool> isReady;

      int run();
      void kill();
      void restart();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };

  std::atomic<bool> App::isReady {false};

  class Window : public IWindow {
    GtkWidget* window;
    GtkWidget* webview;
    GtkWidget* vbox;
    GtkWidget* popup;
    int popupId;

    public:
      App app;
      WindowOptions opts;
      Window(App&, WindowOptions);
      bool webviewFailed = false;

      void about();
      void eval(const std::string&);
      void show(const std::string&);
      void hide(const std::string&);
      void kill();
      void close();
      void exit();
      void navigate(const std::string&, const std::string&);
      void setTitle(const std::string&, const std::string&);
      void setSize(const std::string&, int, int, int);
      void setContextMenu(const std::string&, const std::string&);
      void closeContextMenu(const std::string&);
      void closeContextMenu();
      void openDialog(const std::string&, bool, bool, bool, bool, const std::string&, const std::string&);
      ScreenSize getScreenSize();

      void setSystemMenu(const std::string& seq, const std::string& menu);
      int openExternal(const std::string& s);
  };

  App::App (int instanceId) {
    gtk_init_check(0, nullptr);
    // TODO enforce single instance is set
  }

  int App::run () {
    gtk_main();
    return shouldExit ? 1 : 0;
  }

  void App::kill () {
    shouldExit = true;
    gtk_main_quit();
  }

  void App::restart () {
  }

  std::string App::getCwd(const std::string &s) {
    auto canonical = fs::canonical("/proc/self/exe");
    return std::string(fs::path(canonical).parent_path());
  }

  void App::dispatch(std::function<void()> f) {
    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* f) -> int {
        (*static_cast<std::function<void()>*>(f))();
        return G_SOURCE_REMOVE;
      }),
      new std::function<void()>(f),
      [](void* f) {
        delete static_cast<std::function<void()>*>(f);
      }
    );
  }

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    setenv("GTK_OVERLAY_SCROLLING", "1", 1);

    popupId = 0;
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    popup = nullptr;

    if (opts.resizable) {
      gtk_window_set_default_size(GTK_WINDOW(window), opts.width, opts.height);
    } else {
      gtk_widget_set_size_request(window, opts.width, opts.height);
    }

    gtk_window_set_resizable(GTK_WINDOW(window), opts.resizable);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

    WebKitUserContentManager* cm = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(cm, "external");

    g_signal_connect(
      cm,
      "script-message-received::external",
      G_CALLBACK(+[](WebKitUserContentManager*, WebKitJavascriptResult* r, gpointer arg) {
        auto *w = static_cast<Window*>(arg);
        JSCValue* value = webkit_javascript_result_get_js_value(r);
        std::string str = std::string(jsc_value_to_string(value));
        if (w->onMessage != nullptr) w->onMessage(str);
      }),
      this
    );

    webview = webkit_web_view_new_with_user_content_manager(cm);

    g_signal_connect(
      G_OBJECT(webview),
      "load-changed",
      G_CALLBACK(+[](WebKitWebView*, WebKitLoadEvent event, gpointer arg) {
        auto *w = static_cast<Window*>(arg);

        if (event == WEBKIT_LOAD_FINISHED) {
          w->app.isReady = true;
        }
      }),
      this
    );

    g_signal_connect(
      G_OBJECT(window),
      "destroy",
      G_CALLBACK(+[](GtkWidget*, gpointer arg) {
        static_cast<Window*>(arg)->close();
      }),
      this
    );

    std::string preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.external.postMessage(arg)\n"
      "};\n"
      "" + createPreload(opts) + "\n"
    );


    WebKitUserContentManager *manager =
      webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));

    webkit_user_content_manager_add_script(
      manager,
      webkit_user_script_new(
        preload.c_str(),
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr,
        nullptr
      )
    );

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, true);

    if (this->opts.forwardConsole) {
      webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
    }

    if (this->opts.debug) {
      webkit_settings_set_enable_developer_extras(settings, true);
    }

    if (this->opts.isTest) {
      webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
      webkit_settings_set_allow_file_access_from_file_urls(settings, true);
    }

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_end(GTK_BOX(vbox), webview, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_add_events(window, GDK_ALL_EVENTS_MASK);

    gtk_widget_grab_focus(GTK_WIDGET(webview));
  }

  ScreenSize Window::getScreenSize () {
    auto* display = gdk_display_get_default();
    auto* win = gtk_widget_get_window(window);
    auto* mon = gdk_display_get_monitor_at_window(display, win);

    GdkRectangle workarea = {0};
    gdk_monitor_get_geometry(mon, &workarea);

    return ScreenSize {
      .height = (int) workarea.height,
      .width = (int) workarea.width
    };
  }

  void Window::eval(const std::string& s) {
    webkit_web_view_run_javascript(
      WEBKIT_WEB_VIEW(webview),
      s.c_str(),
      nullptr,
      nullptr,
      nullptr
    );
  }

  void Window::show(const std::string &seq) {
    gtk_widget_show_all(window);

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  void Window::hide(const std::string &seq) {
    gtk_widget_hide(window);

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  void Window::exit() {
    if (onExit != nullptr) onExit();
  }

  void Window::kill() {
    // gtk releases objects automatically.
  }

  void Window::close () {
    if (opts.canExit) {
      this->exit();
    }
  }

  void Window::navigate(const std::string &seq, const std::string &s) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), s.c_str());

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  void Window::setTitle(const std::string &seq, const std::string &s) {
    gtk_window_set_title(GTK_WINDOW(window), s.c_str());

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  int Window::openExternal(const std::string& url) {
    return gtk_show_uri_on_window(GTK_WINDOW(window), url.c_str(), GDK_CURRENT_TIME, nullptr);
  }


  void Window::about () {
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 200);

    GtkWidget *body = gtk_dialog_get_content_area(GTK_DIALOG(GTK_WINDOW(dialog)));
    GtkContainer *content = GTK_CONTAINER(body);

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
      "/usr/share/icons/hicolor/256x256/apps/operator.png",
      60,
      60,
      TRUE,
      nullptr
    );

    GtkWidget *img = gtk_image_new_from_pixbuf(pixbuf);
    gtk_widget_set_margin_top(img, 20);
    gtk_widget_set_margin_bottom(img, 20);

    gtk_box_pack_start(GTK_BOX(content), img, FALSE, FALSE, 0);

    std::string title_value(appData["title"] + " " + appData["version"]);

    GtkWidget *label_title = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_title), title_value.c_str());
    gtk_container_add(content, label_title);

    GtkWidget *label_copyRight = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label_copyRight), appData["copyRight"].c_str());
    gtk_container_add(content, label_copyRight);

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

  void Window::setSize(const std::string& seq, int width, int height, int hints) {
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

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  void Window::setSystemMenu(const std::string &seq, const std::string &value) {
    if (value.empty()) return void(0);

    auto menu = std::string(value);

    GtkWidget *menubar = gtk_menu_bar_new();
    GtkAccelGroup *acclg = gtk_accel_group_new();

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
        std::string key = "";

        GtkWidget *item;
        GtkWidget *accel;
        GdkModifierType mods;

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
              bool isShift = std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;

              if (accelerator.size() > 1) {
                if (accelerator[1].find("Meta") != -1) {
                  mask = (GdkModifierType)(mask | GDK_META_MASK);
                } else if (accelerator[1].find("CommandOrControl") != -1) {
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

              auto ag = gtk_accel_group_new();

              gtk_widget_add_accelerator(
                item,
                "activate",
                ag,
                (char) key[0],
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

              if (std::string(title).find("About") == 0) {
                return w->about();
              }

              if (std::string(title).find("Quit") == 0) {
                return w->exit();
              }

              w->resolveMenuSelection("0", title, parent);
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

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_widget_show_all(window);

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      resolveToMainProcess(seq, "0", index);
    }
  }

  void Window::closeContextMenu() {
    if (popupId > 0) {
      closeContextMenu(std::to_string(popupId));
    }
  }

  void Window::closeContextMenu(const std::string &seq) {
    auto ptr = popup;
    if (ptr != nullptr) {
      popupId = 0;
      popup = nullptr;
      gtk_menu_popdown((GtkMenu *) ptr);
      gtk_widget_destroy(ptr);
      resolveMenuSelection(seq, "", "contextMenu");
    }
  }

  void Window::setContextMenu(const std::string &seq, const std::string &value) {
    closeContextMenu();

    // members
    popup = gtk_menu_new();
    popupId = std::stoi(seq);

    auto menuData = replace(value, "_", "\n");
    auto menuItems = split(menuData, '\n');
    GtkWidget *item;

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
      auto meta = std::string(seq + ";" + pair[0].c_str());
      auto *item = gtk_menu_item_new_with_label(pair[0].c_str());

      g_signal_connect(
        G_OBJECT(item),
        "activate",
        G_CALLBACK(+[](GtkWidget *t, gpointer arg) {
          auto window = static_cast<Window*>(arg);
          auto label = gtk_menu_item_get_label(GTK_MENU_ITEM(t));
          auto title = std::string(label);
          auto meta = gtk_widget_get_name(t);
          auto pair = split(meta, ';');
          auto seq = pair[0];

          window->resolveMenuSelection(seq, title, "contextMenu");
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
    event->button.window = g_object_ref(win);
    event->button.time = GDK_CURRENT_TIME - 100;

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
    const std::string& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const std::string& defaultPath,
    const std::string& title
  ) {

    GtkWidget *dialog;
    GtkFileFilter *filter;
    GtkFileChooser *chooser;
    GtkFileChooserAction action;
    gint res;
    char buf[128], *patterns;

    if (isSave) {
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
    } else {
      action = GTK_FILE_CHOOSER_ACTION_OPEN;
    }

    if (isSave || allowDirs) {
      action = (GtkFileChooserAction) (action | GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    }

    gtk_init_check(nullptr, nullptr);

    dialog = gtk_file_chooser_dialog_new(
      isSave ? "Save File" : "Open File",
      nullptr,
      action,
      "_Cancel",
      GTK_RESPONSE_CANCEL,
      isSave ? "_Save" : "_Open",
      GTK_RESPONSE_ACCEPT,
      nullptr
    );

    chooser = GTK_FILE_CHOOSER(dialog);

    // if (FILE_DIALOG_OVERWRITE_CONFIRMATION) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    // }

    // TODO (@heapwolf): make optional
    if ((!isSave || allowDirs) && allowMultiple) {
      gtk_file_chooser_set_select_multiple(chooser, TRUE);
    }

    if (defaultPath.size() > 0) {
      gtk_file_chooser_set_filename(chooser, defaultPath.c_str());
    }

    if (title.size() > 0) {
      gtk_file_chooser_set_current_name(chooser, title.c_str());
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
      gtk_widget_destroy(dialog);
      return;
    }

    // TODO (@heapwolf): validate multi-select

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }

    std::string result("");
    GSList* filenames = gtk_file_chooser_get_filenames(chooser);
    int i = 0;

    while (filenames != nullptr) {
      gchar* file = (gchar*) filenames->data;
      result += (i++ ? "\\n" : "");
      result += std::string(file);
      filenames = filenames->next;
    }

    g_slist_free(filenames);

    auto wrapped =  std::string("\"" + std::string(result) + "\"");
    resolveToRenderProcess(seq, "0", encodeURIComponent(wrapped));
    gtk_widget_destroy(dialog);
  }
}
