#include "platform.h"
#include <gtk/gtk.h>
#include <filesystem>

namespace fs = std::filesystem;

std::string get_cwd () {
  std::string _path = fs::current_path();
  std::string cwd = fs::absolute(_path);

  return cwd;
}

void createMenu(std::string str) {
  // TODO
}

bool createContextMenu(std::string a, std::string b) {
  // TODO
}

std::string getCwd() {
  // TODO
  return "";
}

const char *noc_file_dialog_open(int flags,
  const char *filters,
  const char *default_path,
  const char *default_name)
{
  GtkWidget *dialog;
  GtkFileFilter *filter;
  GtkFileChooser *chooser;
  GtkFileChooserAction action;
  gint res;
  char buf[128], *patterns;

  action = flags & NOC_FILE_DIALOG_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE :
                                          GTK_FILE_CHOOSER_ACTION_OPEN;
  if (flags & NOC_FILE_DIALOG_DIR)
      action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

  gtk_init_check(NULL, NULL);
  dialog = gtk_file_chooser_dialog_new(
          flags & NOC_FILE_DIALOG_SAVE ? "Save File" : "Open File",
          NULL,
          action,
          "_Cancel", GTK_RESPONSE_CANCEL,
          flags & NOC_FILE_DIALOG_SAVE ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
          NULL );
  chooser = GTK_FILE_CHOOSER(dialog);
  if (flags & NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION)
      gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

  if (default_path)
      gtk_file_chooser_set_filename(chooser, default_path);
  if (default_name)
      gtk_file_chooser_set_current_name(chooser, default_name);

  while (filters && *filters) {
      filter = gtk_file_filter_new();
      gtk_file_filter_set_name(filter, filters);
      filters += strlen(filters) + 1;

      // Split the filter pattern with ';'.
      strcpy(buf, filters);
      buf[strlen(buf)] = '\0';
      for (patterns = buf; *patterns; patterns++)
          if (*patterns == ';') *patterns = '\0';
      patterns = buf;
      while (*patterns) {
          gtk_file_filter_add_pattern(filter, patterns);
          patterns += strlen(patterns) + 1;
      }

      gtk_file_chooser_add_filter(chooser, filter);
      filters += strlen(filters) + 1;
  }

  res = gtk_dialog_run(GTK_DIALOG(dialog));

  // free(res);
  // res = NULL;

  // if (res == GTK_RESPONSE_ACCEPT)
  //    g_noc_file_dialog_ret = gtk_file_chooser_get_filename(chooser);
  gtk_widget_destroy(dialog);
  while (gtk_events_pending()) gtk_main_iteration();
  return ""; // res; // g_noc_file_dialog_ret;
}


