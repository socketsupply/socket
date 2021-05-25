#ifndef PLATFORM_H
#define PLATFORM_H

#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define OS_WIN32
#endif

#ifdef __APPLE__
#define OS_DARWIN
#endif

#ifdef __linux__
#define OS_LINUX
#endif

#include <string>
#include <vector>

struct Platform {
#ifdef OS_LINUX
  bool darwin = false;
  bool win32 = false;
  bool linux = true;
  const std::string os = "linux";
#endif

#ifdef OS_DARWIN
  bool darwin = true;
  bool win32 = false;
  bool linux = false;
  const std::string os = "darwin";
#endif

#ifdef OS_WIN32
  bool darwin = false;
  bool win32 = true;
  bool linux = false;
  const std::string os = "win32";
#endif
};

inline const std::vector<std::string> split(const std::string& s, const char& c) {
  std::string buff;
  std::vector<std::string> vec;
	
	for (auto n : s) {
		if(n != c) {
      buff += n;
    } else if (n == c && buff != "") {
      vec.push_back(buff);
      buff = "";
    }
	}

	if (!buff.empty()) vec.push_back(buff);

	return vec;
}

inline std::string& trim(std::string& str) {
  str.erase(0, str.find_first_not_of(" \r\n\t"));
  str.erase(str.find_last_not_of(" \r\n\t") + 1);
  return str;
}

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
std::string createNativeDialog(
  int flags,
  const char *filters,
  const char *default_path,
  const char *default_name);

std::string getCwd (std::string);

#endif /* PLATFORM_H */
