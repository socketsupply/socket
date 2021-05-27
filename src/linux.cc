#include "util.h"
#include <gtk/gtk.h>
#include <filesystem>

namespace fs = std::filesystem;

std::string getCwd (std::string argvp) {
  auto canonical = fs::canonical("/proc/self/exe");
  return std::string(fs::path(canonical).parent_path());
}

std::string createNativeDialog(
  int flags,
  const char *_,
  const char *default_path,
  const char *default_name)
  {

  GtkWidget *dialog;
  GtkFileFilter *filter;
  GtkFileChooser *chooser;
  GtkFileChooserAction action;
  gint res;
  char buf[128], *patterns;

  action = flags & NOC_FILE_DIALOG_SAVE
    ? GTK_FILE_CHOOSER_ACTION_SAVE
    : GTK_FILE_CHOOSER_ACTION_OPEN;

  if (flags & NOC_FILE_DIALOG_DIR) {
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  }

  gtk_init_check(NULL, NULL);

  dialog = gtk_file_chooser_dialog_new(
    flags & NOC_FILE_DIALOG_SAVE
      ? "Save File"
      : "Open File",
    NULL,
    action,
    "_Cancel",
    GTK_RESPONSE_CANCEL,
    flags & NOC_FILE_DIALOG_SAVE
      ? "_Save"
      : "_Open",
    GTK_RESPONSE_ACCEPT,
    NULL
  );

  chooser = GTK_FILE_CHOOSER(dialog);

  if (flags & NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION) {
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
  }

  if (default_path != nullptr) {
    gtk_file_chooser_set_filename(chooser, default_path);
  }

  if (default_name != nullptr) {
    gtk_file_chooser_set_current_name(chooser, default_name);
  }

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
    return std::string("");
  }

  auto filename = gtk_file_chooser_get_filename(chooser);

  gtk_widget_destroy(dialog);

  while (gtk_events_pending()) {
    gtk_main_iteration();
  }

  return std::string(filename);
}
