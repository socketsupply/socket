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
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };

  std::atomic<bool> App::isReady {false};

  class Window : public IWindow {
    GtkWidget* window;
    GtkWidget *webview;

    public:
      App app;
      WindowOptions opts;
      Window(App&, WindowOptions);

      void eval(const std::string&);
      void show(const std::string&);
      void hide(const std::string&);
      void kill();
      void exit();
      void navigate(const std::string&, const std::string&);
      void setTitle(const std::string&, const std::string&);
      void setSize(int, int, int);
      void setContextMenu(const std::string&, const std::string&);
      void openDialog(const std::string&, bool, bool, bool, const std::string&, const std::string&);

      void setSystemMenu(const std::string& seq, const std::string& menu);
      int openExternal(const std::string& s);
  };

  App::App (int instanceId) {
    gtk_init_check(0, NULL);
    // TODO enforce single instance is set
  }

  int App::run () {
    // gtk_main();
    gtk_main_iteration_do(true);
    return shouldExit;
  }

  void App::kill () {
    gtk_main_quit();
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

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    if (opts.resizable) {
      gtk_window_set_default_size(GTK_WINDOW(window), opts.width, opts.height);
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

    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webview));

    g_signal_connect(
      G_OBJECT(window),
      "destroy",
      G_CALLBACK(+[](GtkWidget*, gpointer arg) {
        static_cast<Window*>(arg)->exit();
      }),
      this
    );

    std::string preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.external.postMessage(arg)\n"
      "};\n"
      "" + createPreload() + "\n"
    );

    webkit_web_view_run_javascript(
      WEBKIT_WEB_VIEW(webview),
      preload.c_str(),
      NULL,
      NULL,
      NULL
    );

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, true);

    if (this->opts.debug) {
      webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
      webkit_settings_set_enable_developer_extras(settings, true);
    }

    gtk_widget_grab_focus(GTK_WIDGET(webview));
    gtk_widget_show_all(window);
  }

  void Window::eval(const std::string& s) {
    webkit_web_view_run_javascript(
      WEBKIT_WEB_VIEW(webview),
      s.c_str(),
      NULL,
      NULL,
      NULL
    );
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
    return gtk_show_uri_on_window(GTK_WINDOW(window), url.c_str(), GDK_CURRENT_TIME, NULL);
  }

  void Window::setSize(int width, int height, int hints) {
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
  }

  void Window::openDialog (
      const std::string& seq,
      bool isSave,
      bool allowDirs,
      bool allowFiles,
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

    if (allowDirs) {
      // action += GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    }

    gtk_init_check(NULL, NULL);

    dialog = gtk_file_chooser_dialog_new(
      isSave ? "Save File" : "Open File",
      NULL,
      action,
      "_Cancel",
      GTK_RESPONSE_CANCEL,
      isSave ? "_Save" : "_Open",
      GTK_RESPONSE_ACCEPT,
      NULL
    );

    chooser = GTK_FILE_CHOOSER(dialog);

    // if (FILE_DIALOG_OVERWRITE_CONFIRMATION) {
      gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    // }

    if (defaultPath.size() > 0) {
      gtk_file_chooser_set_filename(chooser, defaultPath.c_str());
    }

    if (title.size() > 0) {
      gtk_file_chooser_set_current_name(chooser, title.c_str());
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
      return;
    }

    // TODO (@heapwolf): validate multi-select
    auto result = gtk_file_chooser_get_filename(chooser);

    gtk_widget_destroy(dialog);

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }

    auto wrapped =  std::string("\"" + std::string(result) + "\"");
    resolveToRenderProcess(seq, "0", wrapped);
  }
}
