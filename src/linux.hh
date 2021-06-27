#include "common.hh"

#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

namespace Opkit { 

  class App : public IApp {
    

    public:
      App(int);
      int run();
      void kill();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };

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
    gtk_main();
  }

  void App::kill () {
    gtk_main_quit();
  }

  void App::dispatch(std::function<void()> f) {
    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* f) -> int {
        (*static_cast<dispatch_fn_t*>(f))();
        return G_SOURCE_REMOVE;
      }),
      new std::function<void()>(f),
      [](void* f) {
        delete static_cast<dispatch_fn_t *>(f);
      }
    );
  }

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    setenv("GTK_OVERLAY_SCROLLING", "1", 1);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    if (opts.resizable) {
      gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    } else {
      gtk_widget_set_size_request(window, width, height);
    }

    gtk_window_set_resizable(GTK_WINDOW(window), opts.resizable);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget* container = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(window), container);

    WebKitUserContentManager* cm = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(cm, "external");

    g_signal_connect(
      cm,
      "script-message-received::external",
      G_CALLBACK(external_message_received_cb),
      this
    );

    webview = webkit_web_view_new_with_user_content_manager(cm);

    g_signal_connect(
      G_OBJECT(webview),
      "load-changed",
      G_CALLBACK(webview_load_changed_cb),
      this
    );

    gtk_container_add(GTK_CONTAINER(container), webview);

    g_signal_connect(window, "destroy", G_CALLBACK(destroyWindowCb), this);

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
  }

  int Window::openExternal(const std::string& url) {
    return gtk_show_uri_on_window(GTK_WINDOW(m_window), url.c_str(), GDK_CURRENT_TIME, NULL);
  }
}