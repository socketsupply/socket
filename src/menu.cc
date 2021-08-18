#include <gtk/gtk.h>

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  static GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  g_signal_connect(
    G_OBJECT(window),
    "button-press-event",
    G_CALLBACK(+[](gpointer data, GdkEvent *event) {
      const int RIGHT_CLICK = 3;
      GdkEventButton* bevent = (GdkEventButton*) event;
      if (bevent->button != RIGHT_CLICK) return;

      GtkWidget* popup = gtk_menu_new();

      auto item = gtk_menu_item_new_with_label("hello");

      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(popup), item);

      // auto win = GDK_WINDOW(gtk_widget_get_window(window));
      // auto seat = gdk_display_get_default_seat(gdk_display_get_default());
      // auto mouse_device = gdk_seat_get_pointer(seat);

      // auto e = gdk_event_new(GDK_BUTTON_PRESS);
      // e->button.time = GDK_CURRENT_TIME;
      // e->button.window = win;
      // gdk_event_set_device(e, mouse_device);

      gtk_menu_popup_at_pointer(
        GTK_MENU(popup),
        nullptr
      );
  
      gtk_widget_show(popup);
    }),
    window
  );
 
  gtk_widget_show(window);

  gtk_main();
}
