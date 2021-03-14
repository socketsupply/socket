
/* noc_file_dialog library
 *
 * Copyright (c) 2015 Guillaume Chereau <guillaume@noctua-software.com>
 * Copyright (c) 2021 Paolo Fragomeni <paolo@optool.co>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* A portable library to create open and save dialogs on linux, osx and
 * windows.
 *
 * The library define a single function : noc_file_dialog_open.
 * With three different implementations.
 *
 * Usage:
 *
 * The library does not automatically select the implementation, you need to
 * define one of those macros before including this file:
 *
 *  NOC_FILE_DIALOG_GTK
 *  NOC_FILE_DIALOG_WIN32
 *  NOC_FILE_DIALOG_OSX
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdlib.h>
#include <string>

enum {
  NOC_FILE_DIALOG_OPEN    = 1 << 0,   // Create an open file dialog.
  NOC_FILE_DIALOG_SAVE    = 1 << 1,   // Create a save file dialog.
  NOC_FILE_DIALOG_DIR     = 1 << 2,   // Open a directory.
  NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION = 1 << 3,
};

// There is a single function defined.

/* flags            : union of the NOC_FILE_DIALOG_XXX masks.
 * filters          : a list of strings separated by '\0' of the form:
 *                      "name1 reg1 name2 reg2 ..."
 *                    The last value is followed by two '\0'.  For example,
 *                    to filter png and jpeg files, you can use:
 *                      "png\0*.png\0jpeg\0*.jpeg\0"
 *                    You can also separate patterns with ';':
 *                      "jpeg\0*.jpg;*.jpeg\0"
 *                    Set to NULL for no filter.
 * default_path     : the default file to use or NULL.
 * default_name     : the default file name to use or NULL.
 *
 * The function return a C string.  There is no need to free it, as it is
 * managed by the library.  The string is valid until the next call to
 * no_dialog_open.  If the user canceled, the return value is NULL.
 */
std::string dialog_open(int flags,
  const char *filters,
  const char *default_path,
  const char *default_name);


std::string getCwd ();
void createMenu ();
std::vector<std::string> getMenuItemDetails (void* item);

#endif /* PLATFORM_H */
