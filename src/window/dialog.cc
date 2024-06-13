#include "../core/debug.hh"
#include "../app/app.hh"
#include "window.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../platform/android.hh"
#endif

#if SOCKET_RUNTIME_PLATFORM_IOS
@implementation SSCUIPickerDelegate : NSObject
-  (void) documentPicker: (UIDocumentPickerViewController*) controller
  didPickDocumentsAtURLs: (NSArray<NSURL*>*) urls
{
  for (NSURL* url in urls) {
    if (url.isFileURL) {
      self.dialog->delegatedResults.push_back(url.path.UTF8String);
    }
  }
  self.dialog->delegateMutex.unlock();
}

- (void) documentPickerWasCancelled: (UIDocumentPickerViewController*) controller {
  self.dialog->delegateMutex.unlock();
}

-  (void) imagePickerController: (UIImagePickerController*) picker
  didFinishPickingMediaWithInfo: (NSDictionary<UIImagePickerControllerInfoKey, id>*) info
{
  NSURL* mediaURL = info[UIImagePickerControllerMediaURL];
  NSURL* imageURL = info[UIImagePickerControllerImageURL];

  if (mediaURL != nullptr) {
    self.dialog->delegatedResults.push_back(mediaURL.path.UTF8String);
  } else {
    self.dialog->delegatedResults.push_back(imageURL.path.UTF8String);
  }

  self.dialog->delegateMutex.unlock();
  [picker dismissViewControllerAnimated: YES completion: nullptr];
}

- (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker {
  self.dialog->delegateMutex.unlock();
}
@end
#endif

namespace SSC {
  Dialog::Dialog (Window* window)
    : window(window)
  {
  #if defined(__APPLE__) && TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    this->uiPickerDelegate = [SSCUIPickerDelegate new];
    this->uiPickerDelegate.dialog = this;
  #endif
  }

  Dialog::~Dialog () {
  #if defined(__APPLE__) && TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #if !__has_feature(objc_arc)
    [this->uiPickerDelegate release];
    #endif
      this->uiPickerDelegate = nullptr;
  #endif
  }

  String Dialog::showSaveFilePicker (
    const FileSystemPickerOptions& options
  ) {
    const auto results = this->showFileSystemPicker({
      .directories = false,
      .multiple = false,
      .files = true,
      .type = FileSystemPickerOptions::Type::Save,
      .contentTypes = options.contentTypes,
      .defaultName = options.defaultName,
      .defaultPath = options.defaultPath,
      .title = options.title
    });

    if (results.size() == 1) {
      return results[0];
    }

    return "";
  }


  Vector<String> Dialog::showOpenFilePicker (
    const FileSystemPickerOptions& options
  ) {
    return this->showFileSystemPicker({
      .directories = false,
      .multiple = options.multiple,
      .files = true,
      .type = FileSystemPickerOptions::Type::Open,
      .contentTypes = options.contentTypes,
      .defaultName = options.defaultName,
      .defaultPath = options.defaultPath,
      .title = options.title
    });
  }

  Vector<String> Dialog::showDirectoryPicker (
    const FileSystemPickerOptions& options
  ) {
    return this->showFileSystemPicker({
      .directories = true,
      .multiple = options.multiple,
      .files = false,
      .type = FileSystemPickerOptions::Type::Open,
      .contentTypes = options.contentTypes,
      .defaultName = options.defaultName,
      .defaultPath = options.defaultPath,
      .title = options.title
    });
  }

  Vector<String> Dialog::showFileSystemPicker (
    const FileSystemPickerOptions& options
  ) {
    const auto isSavePicker = options.type == FileSystemPickerOptions::Type::Save;
    const auto allowDirectories = options.directories;
    const auto allowMultiple = options.multiple;
    const auto allowFiles = options.files;
    const auto defaultName = options.defaultName;
    const auto defaultPath = options.defaultPath;
    const auto title = options.title;

    Vector<String> paths;

  #if defined(__APPLE__)
    // state
    NSMutableArray<UTType *>* contentTypes = [NSMutableArray new];
    NSString* suggestedFilename = nullptr;
    NSURL* directoryURL = nullptr;
    bool prefersMedia = false;

    if (defaultName.size() > 0 && defaultPath.size() == 0) {
      directoryURL = [NSURL fileURLWithPath: @(defaultName.c_str())];
    } else if (defaultPath.size() > 0) {
      directoryURL = [NSURL fileURLWithPath: @(defaultPath.c_str())];
    }

    if (allowDirectories) {
      [contentTypes addObject: UTTypeFolder];
    }

    if (allowFiles) {
      // <mime>:<ext>,<ext>|<mime>:<ext>|...
      for (const auto& contentTypeSpec : split(options.contentTypes, "|")) {
        const auto parts = split(contentTypeSpec, ":");
        const auto mime = parts[0];
        const auto classes = split(mime, "/");
        UTType* supertype = nullptr;

        // malformed MIME
        if (classes.size() != 2) {
          continue;
        }

        if (classes[0] == "audio") {
          supertype = UTTypeAudio;
          prefersMedia = true;
        } else if (classes[0] == "font") {
          supertype = UTTypeFont;
          prefersMedia = false;
        } else if (classes[0] == "image") {
          supertype = UTTypeImage;
          prefersMedia = true;
        } else if (classes[0] == "text") {
          supertype = UTTypeText;
          prefersMedia = false;
        } else if (classes[0] == "video") {
          supertype = UTTypeVideo;
          prefersMedia = true;
        } else if (classes[0] == "*") {
          supertype = UTTypeData;
        } else {
          supertype = UTTypeCompositeContent;
        }

        // any file extension such that its mime type corresponds to <mime>
        if (parts.size() == 1) {
          if (classes[1] == "*") {
            [contentTypes addObject: supertype];
          } else {
            [contentTypes
              addObjectsFromArray: [UTType
                    typesWithTag: @(mime.c_str())
                        tagClass: UTTagClassMIMEType
                conformingToType: supertype
              ]
            ];
          }
        }

        // given file extensions for a given mime type
        if (parts.size() == 2) {
          const auto extensions = split(parts[1], ",");

          for (const auto& extension : extensions) {
            auto types = [UTType
                  typesWithTag: @(extension.c_str())
                      tagClass: UTTagClassFilenameExtension
              conformingToType: supertype
            ];

            [contentTypes addObjectsFromArray: types];
          }
        }
      }

      if (contentTypes.count == 0 && !allowDirectories) {
        [contentTypes addObject: UTTypeContent];
      }
    }

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    this->delegateMutex.lock();
    dispatch_async(dispatch_get_main_queue(), ^{
      UIWindow* window = nullptr;

      if (@available(iOS 15.0, *)) {
        auto scene = (UIWindowScene*) UIApplication.sharedApplication.connectedScenes.allObjects.firstObject;
        window = scene.windows.lastObject;
      } else {
        window = UIApplication.sharedApplication.windows.lastObject;
      }

      if (prefersMedia) {
        auto picker = [UIImagePickerController new];
        NSMutableArray<NSString*>* mediaTypes = [NSMutableArray new];

        picker.delegate = this->uiPickerDelegate;

        [window.rootViewController
          presentViewController: picker
                       animated: YES
                     completion: nullptr
        ];
      } else {
        auto picker = [UIDocumentPickerViewController.alloc
          initForOpeningContentTypes: contentTypes
        ];

        picker.allowsMultipleSelection = allowMultiple ? YES : NO;
        picker.modalPresentationStyle = UIModalPresentationFormSheet;
        picker.directoryURL = directoryURL;
        picker.delegate = this->uiPickerDelegate;

        [window.rootViewController
          presentViewController: picker
                       animated: YES
                     completion: nullptr
        ];
      }
    });

    std::lock_guard<std::mutex> lock(this->delegateMutex);
    paths = this->delegatedResults;
    this->delegatedResults.clear();
  #else
    NSAutoreleasePool* pool = [NSAutoreleasePool new];

    // dialogs
    NSSavePanel* saveDialog = nullptr;
    NSOpenPanel* openDialog = nullptr;

    if (isSavePicker) {
      saveDialog = [NSSavePanel savePanel];
      saveDialog.allowedContentTypes = contentTypes;
    } else {
      openDialog = [NSOpenPanel openPanel];
      openDialog.allowedContentTypes = contentTypes;
    }

    if (isSavePicker) {
      [saveDialog setCanCreateDirectories: YES];
    } else {
      [openDialog setCanCreateDirectories: YES];
    }

    if (allowDirectories == true && isSavePicker == false) {
      [openDialog setCanChooseDirectories: YES];
    }

    if (isSavePicker == false) {
      [openDialog setCanChooseFiles: allowFiles ? YES : NO];
    }

    if ((isSavePicker == false || allowDirectories == true) && allowMultiple == true) {
      if (openDialog != nullptr) {
        [openDialog setAllowsMultipleSelection: YES];
      }
    }

    if (defaultName.size() > 0) {
      suggestedFilename = @(defaultName.c_str());
    } else if (defaultName.size() == 0 && defaultPath.size() > 0 && directoryURL != nullptr) {
      suggestedFilename = directoryURL.lastPathComponent;
    }

    if (directoryURL != nullptr) {
      if (isSavePicker) {
        [saveDialog setDirectoryURL: directoryURL];
        [saveDialog setNameFieldStringValue: suggestedFilename];
      } else {
        [openDialog setDirectoryURL: directoryURL];
        [openDialog setNameFieldStringValue: suggestedFilename];
      }
    }

    if (title.size() > 0) {
      if (isSavePicker) {
        [saveDialog setTitle: @(title.c_str())];
      } else {
        [openDialog setTitle: @(title.c_str())];
      }
    }

    if (saveDialog != nullptr) {
      if ([saveDialog runModal] == NSModalResponseOK) {
        paths.push_back(saveDialog.URL.path.UTF8String);
      }
    } else if ([openDialog runModal] == NSModalResponseOK) {
      for (NSURL* url in openDialog.URLs) {
        if (url.isFileURL) {
          paths.push_back(url.path.UTF8String);
        }
      }
    }

    [pool release];
  #endif
  #elif defined(__linux__) && !defined(__ANDROID__)
    const guint SELECT_RESPONSE = 0;
    GtkFileChooserAction action;
    GtkFileChooser *chooser;
    GtkFileFilter *filter;
    GtkWidget *dialog;
    Vector<GtkFileFilter*> filters;

    GtkFileFilterFunc wildcardFilter = [](const GtkFileFilterInfo* info, gpointer userData) -> gboolean {
      if (info != nullptr && info->mime_type != nullptr) {
        const auto mimeType = String(info->mime_type);
        if (userData != nullptr) {
          const auto targetMimeType = String((char*) userData);
          if (mimeType.starts_with(targetMimeType)) {
            return TRUE;
          }
        }
      }

      return FALSE;
    };

    for (const auto& contentTypeSpec : split(options.contentTypes, "|")) {
      const auto parts = split(contentTypeSpec, ":");
      const auto mime = parts[0];
      const auto classes = split(mime, "/");

      // malformed MIME
      if (classes.size() != 2) {
        continue;
      }

      auto filter = gtk_file_filter_new();

    #define MAKE_FILTER(userData)                                              \
      gtk_file_filter_add_custom(                                              \
        filter,                                                                \
        GTK_FILE_FILTER_MIME_TYPE,                                             \
        wildcardFilter,                                                        \
        (gpointer) userData,                                                   \
        NULL                                                                   \
      );

      if (classes[1] != "*") {
        if (classes[0] == "audio") {
          MAKE_FILTER("audio");
        } else if (classes[0] == "font") {
          MAKE_FILTER("font");
        } else if (classes[0] == "image") {
          MAKE_FILTER("images");
        } else if (classes[0] == "text") {
          MAKE_FILTER("text");
        } else if (classes[0] == "video") {
          MAKE_FILTER("video");
        }
      } else {
        gtk_file_filter_add_mime_type(filter, mime.c_str());
      }

      // given file extensions for a given mime type
      if (parts.size() == 2) {
        const auto extensions = split(parts[1], ",");

        for (const auto& extension : extensions) {
          const auto pattern = (
            String("*") +
            (!extension.starts_with(".") ? "." : "") +
            extension
          );

          gtk_file_filter_add_pattern(filter, pattern.c_str());
        }
      }

      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
      filters.push_back(filter);
    }

    if (isSavePicker) {
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
    } else {
      action = GTK_FILE_CHOOSER_ACTION_OPEN;
    }

    if (!allowFiles && allowDirectories) {
      action = (GtkFileChooserAction) (action | GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    }

    gtk_init_check(nullptr, nullptr);

    String dialogTitle = isSavePicker ? "Save File" : "Open File";
    if (title.size() > 0) {
      dialogTitle = title;
    }

    dialog = gtk_file_chooser_dialog_new(
      dialogTitle.c_str(),
      nullptr,
      action,
      "_Cancel",
      GTK_RESPONSE_CANCEL,
      nullptr
    );

    chooser = GTK_FILE_CHOOSER(dialog);

    if (!allowDirectories) {
      if (isSavePicker) {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Save", GTK_RESPONSE_ACCEPT);
      } else {
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Open", GTK_RESPONSE_ACCEPT);
      }
    }

    if (allowMultiple || allowDirectories) {
      gtk_dialog_add_button(GTK_DIALOG(dialog), "Select", SELECT_RESPONSE);
    }

    gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);

    if ((!isSavePicker || allowDirectories) && allowMultiple) {
      gtk_file_chooser_set_select_multiple(chooser, true);
    }

    if (defaultPath.size() > 0) {
      auto status = fs::status(defaultPath);

      if (fs::exists(status)) {
        if (fs::is_directory(status)) {
          gtk_file_chooser_set_current_folder(chooser, defaultPath.c_str());
        } else {
          gtk_file_chooser_set_filename(chooser, defaultPath.c_str());
        }
      }
    }

    if (defaultName.size() > 0) {
      if ((!allowFiles && allowDirectories) || isSavePicker) {
        gtk_file_chooser_set_current_name(chooser, defaultName.c_str());
      } else {
        gtk_file_chooser_set_current_folder(chooser, defaultName.c_str());
      }
    }

    guint response = gtk_dialog_run(GTK_DIALOG(dialog));

    for (const auto& filter : filters) {
      g_object_unref(filter);
    }

    filters.clear();

    if (response != GTK_RESPONSE_ACCEPT && response != SELECT_RESPONSE) {
      gtk_widget_destroy(dialog);
      return paths;
    }

    // TODO (@heapwolf): validate multi-select

    while (gtk_events_pending()) {
      gtk_main_iteration();
    }

    GSList* filenames = gtk_file_chooser_get_filenames(chooser);

    for (int i = 0; filenames != nullptr; ++i) {
      const auto file = (const char*) filenames->data;
      paths.push_back(file);
      filenames = filenames->next;
    }

    g_slist_free(filenames);
    gtk_widget_destroy(dialog);
  #elif defined(_WIN32)
    IShellItemArray *openResults;
    IShellItem *saveResult;
    DWORD dialogOptions;
    DWORD totalResults;
    HRESULT result;

    // the dialogs as a union, because there can only _one_.
    union {
      IFileSaveDialog *save;
      IFileOpenDialog *open;
    } dialog;

    result = CoInitializeEx(
      NULL,
      COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
    );

    if (FAILED(result)) {
      debug("ERR: CoInitializeEx() failed in 'showFileSystemPicker()'");
      return paths;
    }

    // create IFileDialog instance (IFileOpenDialog or IFileSaveDialog)
    if (isSavePicker) {
      result = CoCreateInstance(
        CLSID_FileSaveDialog,
        NULL,
        CLSCTX_ALL,
        IID_PPV_ARGS(&dialog.save)
      );

      if (FAILED(result)) {
        debug("ERR: CoCreateInstance() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    } else {
      result = CoCreateInstance(
        CLSID_FileOpenDialog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog.open)
      );

      if (FAILED(result)) {
        debug("ERR: CoCreateInstance() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (isSavePicker) {
      result = dialog.save->GetOptions(&dialogOptions);
    } else {
      result = dialog.open->GetOptions(&dialogOptions);
    }

    if (FAILED(result)) {
      debug("ERR: IFileDialog::GetOptions() failed in 'showFileSystemPicker()'");
      CoUninitialize();
      return paths;
    }

    if (allowDirectories == true && allowFiles == false) {
      if (isSavePicker) {
        result = dialog.save->SetOptions(dialogOptions | FOS_PICKFOLDERS);
      } else {
        result = dialog.open->SetOptions(dialogOptions | FOS_PICKFOLDERS);
      }

      if (FAILED(result)) {
        debug("ERR: IFileDialog::SetOptions(FOS_PICKFOLDERS) failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if ((!isSavePicker || (!isSavePicker && allowDirectories)) && allowMultiple) {
      result = dialog.open->SetOptions(dialogOptions | FOS_ALLOWMULTISELECT);

      if (FAILED(result)) {
        debug("ERR: IFileDialog::SetOptions(FOS_ALLOWMULTISELECT) failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (!defaultPath.empty()) {
      IShellItem *defaultFolder;

      auto normalizedDefaultPath = defaultPath;
      std::replace(normalizedDefaultPath.begin(), normalizedDefaultPath.end(), '/', '\\');
      result = SHCreateItemFromParsingName(
        WString(normalizedDefaultPath.begin(), normalizedDefaultPath.end()).c_str(),
        NULL,
        IID_PPV_ARGS(&defaultFolder)
      );

      if (FAILED(result)) {
        debug("ERR: SHCreateItemFromParsingName() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }

      if (isSavePicker) {
        result = dialog.save->SetDefaultFolder(defaultFolder);
      } else {
        result = dialog.open->SetDefaultFolder(defaultFolder);
      }

      if (FAILED(result)) {
        debug("ERR: IFileDialog::SetDefaultFolder() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (!title.empty()) {
      if (isSavePicker) {
        result = dialog.save->SetTitle(
          WString(title.begin(), title.end()).c_str()
        );
      } else {
        result = dialog.open->SetTitle(
          WString(title.begin(), title.end()).c_str()
        );
      }

      if (FAILED(result)) {
        debug("ERR: IFileDialog::SetTitle() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (!defaultName.empty()) {
      if (isSavePicker) {
        result = dialog.save->SetFileName(
          WString(defaultName.begin(), defaultName.end()).c_str()
        );
      } else {
        result = dialog.open->SetFileName(
          WString(defaultName.begin(), defaultName.end()).c_str()
        );
      }

      if (FAILED(result)) {
        debug("ERR: IFileDialog::SetFileName() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (isSavePicker) {
      result = dialog.save->Show(NULL);
    } else {
      result = dialog.open->Show(NULL);
    }

    if (FAILED(result)) {
      debug("ERR: IFileDialog::Show() failed in 'showFileSystemPicker()'");
      CoUninitialize();
      return paths;
    }

    if (isSavePicker) {
      result = dialog.save->GetResult(&saveResult);

      if (FAILED(result)) {
        debug("ERR: IFileDialog::GetResult() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    } else {
      result = dialog.open->GetResults(&openResults);

      if (FAILED(result)) {
        debug("ERR: IFileDialog::GetResults() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }
    }

    if (FAILED(result)) {
      debug("ERR: IFileDialog::Show() failed in 'showFileSystemPicker()'");
      CoUninitialize();
      return paths;
    }

    if (isSavePicker) {
      LPWSTR buf;

      result = saveResult->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &buf);

      if (FAILED(result)) {
        debug("ERR: IShellItem::GetDisplayName() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }

      paths.push_back(SSC::convertWStringToString(WString(buf)));
      saveResult->Release();

      CoTaskMemFree(buf);
    } else {
      openResults->GetCount(&totalResults);

      if (FAILED(result)) {
        debug("ERR: IShellItemArray::GetCount() failed in 'showFileSystemPicker()'");
        CoUninitialize();
        return paths;
      }

      for (DWORD i = 0; i < totalResults; i++) {
        IShellItem *path;
        LPWSTR buf;

        result = openResults->GetItemAt(i, &path);

        if (FAILED(result)) {
          debug("ERR: IShellItemArray::GetItemAt() failed in 'showFileSystemPicker()'");
          CoUninitialize();
          return paths;
        }

        result = path->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &buf);

        if (FAILED(result)) {
          debug("ERR: IShellItem::GetDisplayName() failed in 'showFileSystemPicker()'");
          CoUninitialize();
          return paths;
        }

        paths.push_back(SSC::convertWStringToString(WString(buf)));
        path->Release();
        CoTaskMemFree(buf);
      }
    }

    if (isSavePicker) {
      dialog.save->Release();
    } else {
      dialog.open->Release();
    }

    if (!isSavePicker) {
      openResults->Release();
    }

    CoUninitialize();
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    if (this->window->androidWindowRef) {
      const auto app = App::sharedApplication();
      const auto attachment = Android::JNIEnvironmentAttachment(app->jvm);
      const auto dialogRef = CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->window->androidWindowRef,
        "getDialog",
        "()Lsocket/runtime/window/Dialog;"
      );

      String mimeTypes;
      // <mime>:<ext>,<ext>|<mime>:<ext>|...
      for (const auto& contentTypeSpec : split(options.contentTypes, "|")) {
        const auto parts = split(contentTypeSpec, ":");
        const auto mime = parts[0];
        const auto classes = split(mime, "/");
        if (classes.size() == 2) {
          if (mimeTypes.size() == 0) {
            mimeTypes = mime;
          } else {
            mimeTypes += "|" + mime;
          }
        }
      }

      const auto mimeTypesRef = attachment.env->NewStringUTF(mimeTypes.c_str());
      const auto results = (jobjectArray) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        dialogRef,
        "showFileSystemPicker",
        "(Ljava/lang/String;ZZZ)[Landroid/net/Uri;",
        mimeTypesRef,
        allowDirectories,
        allowMultiple,
        allowFiles
      );

      const auto length = attachment.env->GetArrayLength(results);
      for (int i = 0; i < length; ++i) {
        const auto uri = (jstring) attachment.env->GetObjectArrayElement(results, i);
        if (uri) {
          const auto string = Android::StringWrap(attachment.env, CallObjectClassMethodFromAndroidEnvironment(
            attachment.env,
            uri,
            "toString",
            "()Ljava/lang/String;"
          )).str();

          paths.push_back(string);
        }
      }
    }
  #endif

    return paths;
  }
}
