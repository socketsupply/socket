#ifndef SOCKET_RUNTIME_WINDOW_DIALOG_H
#define SOCKET_RUNTIME_WINDOW_DIALOG_H

#include "../platform/platform.hh"

namespace SSC {
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
  @property (nonatomic) SSC::Dialog* dialog;

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

namespace SSC {
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

    #if SOCKET_RUNTIME_PLATFORM_IOS
      SSCUIPickerDelegate* uiPickerDelegate = nullptr;
      Vector<String> delegatedResults;
      std::mutex delegateMutex;
    #endif

      Window* window = nullptr;
      Dialog (Window* window);
      Dialog () = default;
      Dialog (const Dialog&) = delete;
      Dialog (Dialog&&) = delete;
      ~Dialog ();

      Dialog& operator = (const Dialog&) = delete;
      Dialog& operator = (Dialog&&) = delete;

      String showSaveFilePicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showOpenFilePicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showDirectoryPicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showFileSystemPicker (
        const FileSystemPickerOptions& options
      );
  };
}
#endif
