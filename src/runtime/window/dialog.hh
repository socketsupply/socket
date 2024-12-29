#ifndef SOCKET_RUNTIME_WINDOW_DIALOG_H
#define SOCKET_RUNTIME_WINDOW_DIALOG_H

#include "../platform.hh"
#include "../context.hh"

namespace ssc::runtime::window {
  // forward
  class Window;
  class Dialog;
}

#if SOCKET_RUNTIME_PLATFORM_IOS
@interface SSCUIPickerDelegate : NSObject<
  UIDocumentPickerDelegate,

  // TODO(@jwerle): use 'PHPickerViewControllerDelegate' instead
  UIImagePickerControllerDelegate,
  UINavigationControllerDelegate
>
  @property (nonatomic) ssc::runtime::window::Dialog* dialog;

  // UIDocumentPickerDelegate
  - (void) documentPicker: (UIDocumentPickerViewController*) controller
   didPickDocumentsAtURLs: (NSArray<NSURL*>*) urls;
  - (void) documentPickerWasCancelled: (UIDocumentPickerViewController*) controller;

  // UIImagePickerControllerDelegate
  - (void) imagePickerController: (UIImagePickerController*) picker
   didFinishPickingMediaWithInfo: (NSDictionary<UIImagePickerControllerInfoKey, id>*) info;
  - (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker;
@end
#endif

namespace ssc::runtime::window {
  class Dialog {
    public:
      struct FileSystemPickerOptions {
        enum class Type { Open, Save };
        bool prefersDarkMode = false;
        bool directories = false;
        bool multiple = false;
        bool files = false;
        Type type = Type::Open;
        String contentTypes;
        String defaultName;
        String defaultPath;
        String title;
      };

      using ShowCallback = Function<void(Vector<String>)>;

    #if SOCKET_RUNTIME_PLATFORM_IOS
      SSCUIPickerDelegate* uiPickerDelegate = nullptr;
      Vector<String> delegatedResults;
      std::mutex delegateMutex;
    #endif

      ShowCallback callback = nullptr;
      Window* window = nullptr;

      Dialog (Window* window);
      Dialog () = default;
      Dialog (const Dialog&) = delete;
      Dialog (Dialog&&) = delete;
      ~Dialog ();

      Dialog& operator = (const Dialog&) = delete;
      Dialog& operator = (Dialog&&) = delete;

      bool showSaveFilePicker (const FileSystemPickerOptions&, const ShowCallback);
      bool showOpenFilePicker (const FileSystemPickerOptions&, const ShowCallback);
      bool showDirectoryPicker (const FileSystemPickerOptions&, const ShowCallback);
      bool showFileSystemPicker (const FileSystemPickerOptions&, const ShowCallback);
  };
}
#endif
