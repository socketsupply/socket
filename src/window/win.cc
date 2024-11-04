#include "../app/app.hh"
#include "window.hh"
#include <winuser.h>

using namespace Microsoft::WRL;

extern BOOL ChangeWindowMessageFilterEx (
  HWND hwnd,
  UINT message,
  DWORD action,
  void* unused
);

namespace SSC {
  class CDataObject : public IDataObject {
    public:
      HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
      ULONG __stdcall AddRef (void);
      ULONG __stdcall Release (void);

      HRESULT __stdcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
      HRESULT __stdcall GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
      HRESULT __stdcall QueryGetData(FORMATETC *pFormatEtc);
      HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut);
      HRESULT __stdcall SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease);
      HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
      HRESULT __stdcall DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
      HRESULT __stdcall DUnadvise(DWORD dwConnection);
      HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppEnumAdvise);

      CDataObject(FORMATETC *fmt, STGMEDIUM *stgmed, int count);
      ~CDataObject();

    private:
      int LookupFormatEtc (FORMATETC *pFormatEtc);
      LONG m_lRefCount;
      FORMATETC *m_pFormatEtc;
      STGMEDIUM *m_pStgMedium;
      LONG m_nNumFormats;
  };

  CDataObject::CDataObject (FORMATETC *fmtetc, STGMEDIUM *stgmed, int count) {
    m_lRefCount  = 1;
    m_nNumFormats = count;

    m_pFormatEtc  = new FORMATETC[count];
    m_pStgMedium  = new STGMEDIUM[count];

    for(int i = 0; i < count; i++) {
      m_pFormatEtc[i] = fmtetc[i];
      m_pStgMedium[i] = stgmed[i];
    }
  }

  CDataObject::~CDataObject () {
    if(m_pFormatEtc) delete[] m_pFormatEtc;
    if(m_pStgMedium) delete[] m_pStgMedium;
  }

  ULONG __stdcall CDataObject::AddRef (void) {
    return InterlockedIncrement(&m_lRefCount);
  }

  ULONG __stdcall CDataObject::Release (void) {
    LONG count = InterlockedDecrement(&m_lRefCount);

    if (count == 0) {
      delete this;
      return 0;
    } else {
      return count;
    }
  }

  HRESULT __stdcall CDataObject::QueryInterface (REFIID iid, void **ppvObject){
    if (iid == IID_IDataObject || iid == IID_IUnknown) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    } else {
      *ppvObject = 0;
      return E_NOINTERFACE;
    }
  }

  HGLOBAL DupMem (HGLOBAL hMem) {
    DWORD len = GlobalSize(hMem);
    PVOID source = GlobalLock(hMem);
    PVOID dest = GlobalAlloc(GMEM_FIXED, len);

    memcpy(dest, source, len);
    GlobalUnlock(hMem);
    return dest;
  }

  int CDataObject::LookupFormatEtc (FORMATETC *pFormatEtc) {
    for (int i = 0; i < m_nNumFormats; i++) {
      if((pFormatEtc->tymed & m_pFormatEtc[i].tymed)   &&
        pFormatEtc->cfFormat == m_pFormatEtc[i].cfFormat &&
        pFormatEtc->dwAspect == m_pFormatEtc[i].dwAspect) {
        return i;
      }
    }
    return -1;
  }

  HRESULT __stdcall CDataObject::GetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium) {
    int idx;

    if ((idx = LookupFormatEtc(pFormatEtc)) == -1) {
      return DV_E_FORMATETC;
    }

    pMedium->tymed = m_pFormatEtc[idx].tymed;
    pMedium->pUnkForRelease = 0;

    switch(m_pFormatEtc[idx].tymed) {
      case TYMED_HGLOBAL:
        pMedium->hGlobal = DupMem(m_pStgMedium[idx].hGlobal);
        break;

      default:
        return DV_E_FORMATETC;
    }

    return S_OK;
  }

  HRESULT __stdcall CDataObject::GetDataHere (FORMATETC *pFormatEtc, STGMEDIUM *pMedium) {
    return DATA_E_FORMATETC;
  }

  HRESULT __stdcall CDataObject::QueryGetData (FORMATETC *pFormatEtc) {
    return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
  }

  HRESULT __stdcall CDataObject::GetCanonicalFormatEtc (FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut) {
    pFormatEtcOut->ptd = nullptr;
    return E_NOTIMPL;
  }

  HRESULT __stdcall CDataObject::SetData (FORMATETC *pFormatEtc, STGMEDIUM *pMedium,  BOOL fRelease) {
    return E_NOTIMPL;
  }

  HRESULT __stdcall CDataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc) {
    if (dwDirection == DATADIR_GET) {
      return SHCreateStdEnumFmtEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);
    } else {
      return E_NOTIMPL;
    }
  }

  HRESULT __stdcall CDataObject::DAdvise (FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) {
    return OLE_E_ADVISENOTSUPPORTED;
  }

  HRESULT __stdcall CDataObject::DUnadvise (DWORD dwConnection) {
    return OLE_E_ADVISENOTSUPPORTED;
  }

  HRESULT __stdcall CDataObject::EnumDAdvise (IEnumSTATDATA **ppEnumAdvise) {
    return OLE_E_ADVISENOTSUPPORTED;
  }

  HRESULT CreateDataObject (FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, IDataObject **ppDataObject) {
    if(ppDataObject == 0) {
      return E_INVALIDARG;
    }

    *ppDataObject = new CDataObject(fmtetc, stgmeds, count);

    return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
  }

  class CDropSource : public IDropSource {
    public:
      HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
      ULONG __stdcall AddRef(void);
      ULONG __stdcall Release(void);

      HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD keyState);
      HRESULT __stdcall GiveFeedback(DWORD dwEffect);

      CDropSource();
      ~CDropSource();

    private:
      LONG m_lRefCount;
  };

  CDropSource::CDropSource() {
    m_lRefCount = 1;
  }

  CDropSource::~CDropSource() {}

  ULONG __stdcall CDropSource::AddRef(void) {
    return InterlockedIncrement(&m_lRefCount);
  }

  ULONG __stdcall CDropSource::Release(void) {
    LONG count = InterlockedDecrement(&m_lRefCount);

    if (count == 0) {
      delete this;
      return 0;
    } else {
      return count;
    }
  }

  HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject) {
    if (iid == IID_IDropSource || iid == IID_IUnknown) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    } else {
      *ppvObject = 0;
      return E_NOINTERFACE;
    }
  }

  HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD keyState) {
    if (fEscapePressed == TRUE) {
      return DRAGDROP_S_CANCEL;
    }

    if ((keyState & MK_LBUTTON) == 0) {
      return DRAGDROP_S_DROP;
    }

    return S_OK;
  }

  HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect) {
    return DRAGDROP_S_USEDEFAULTCURSORS;
  }

  HRESULT CreateDropSource(IDropSource **ppDropSource) {
    if (ppDropSource == 0) {
      return E_INVALIDARG;
    }

    *ppDropSource = new CDropSource();

    return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;
  }

  class DragDrop : public IDropTarget {
    Vector<String> draggablePayload;
    unsigned int refCount;

    public:
    HWND childWindow;
    Window* window;

    DragDrop(Window* win) : window(win) {
      refCount = 0;
    };

    ~DragDrop () {
      draggablePayload.clear();
    }

    private:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
      if (riid == IID_IUnknown || riid == IID_IDropTarget) {
        *ppv = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
      }
      *ppv = nullptr;
      return E_NOINTERFACE;
    };

    HRESULT STDMETHODCALLTYPE DragEnter(
      IDataObject *dataObject,
      DWORD keyState,
      POINTL dragPoint,
      DWORD *dragEffect
    ) {
      auto parent = this->window->window;
      auto child = this->childWindow;

      auto position = POINT {0};
      auto point = POINT {0};
      auto rect = RECT {0};

      auto format = FORMATETC {0};
      auto medium = STGMEDIUM {0};
      auto result = (HRESULT) 0;

      char *list = 0;

      GetClientRect(child, &rect);
      position = { rect.left, rect.top };

      MapWindowPoints(parent, GetParent(parent), (LPPOINT) &position, 2);
      point.x = dragPoint.x - position.x;
      point.y = dragPoint.y - position.y;

      *dragEffect = DROPEFFECT_MOVE;

      format.cfFormat = CF_TEXT;
      format.ptd      = nullptr;
      format.dwAspect = DVASPECT_CONTENT;
      format.lindex   = -1;
      format.tymed    = TYMED_HGLOBAL;

      dataObject->QueryGetData(&format);
      dataObject->GetData(&format, &medium);

      list = (char *) GlobalLock(medium.hGlobal);

      this->draggablePayload.clear();

      if (list != 0) {
        draggablePayload = split(String(list), ';');


        String json = (
          "{"
          "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
          "  \"inbound\": true,"
          "  \"x\":" + std::to_string(point.x) + ","
          "  \"y\":" + std::to_string(point.y) + ""
          "}"
        );

        auto payload = getEmitToRenderProcessJavaScript("drag", json);
        this->window->eval(payload);
      }

      GlobalUnlock(list);
      ReleaseStgMedium(&medium);

      return S_OK;
    };

    ULONG STDMETHODCALLTYPE AddRef(void) {
      refCount++;

      return refCount;
    }

    ULONG STDMETHODCALLTYPE Release(void) {
      refCount--;

      if (refCount == 0) {
        delete this;
      }

      return refCount;
    }

    HRESULT STDMETHODCALLTYPE DragOver (
      DWORD keyState,
      POINTL dragPoint,
      DWORD *dragEffect
    ) {
      auto position = POINT {0};
      auto parent = this->window->window;
      auto child = this->childWindow;
      auto point = POINT {0};
      auto rect = RECT {0};

      *dragEffect = DROPEFFECT_COPY;

      GetClientRect(child, &rect);
      position = { rect.left, rect.top };

      MapWindowPoints(parent, GetParent(parent), (LPPOINT) &position, 2);
      point.x = dragPoint.x - position.x;
      point.y = dragPoint.y - position.y;

      String json = (
        "{"
        "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
        "  \"inbound\": false,"
        "  \"x\":" + std::to_string(point.x) + ","
        "  \"y\":" + std::to_string(point.y) + ""
        "}"
      );

      auto payload = getEmitToRenderProcessJavaScript("drag", json);
      this->window->eval(payload);

      return S_OK;
    };

    HRESULT STDMETHODCALLTYPE DragLeave(void) {
      // @TODO(jwerle)
      IDropSource* dropSource;
      IDataObject* dataObject;

      DWORD dragDropResult;
      DWORD dropEffect;

      DROPFILES *dropFiles = 0;
      HGLOBAL globalMemory;

      FORMATETC format = { CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_ISTREAM };
      STGMEDIUM medium = { TYMED_HGLOBAL, { 0 }, 0 };
      UINT len = 0;

      Vector<String> files = this->draggablePayload;

      for (auto &file : files) {
        file = file.substr(12);
        len += file.size();
      }

      globalMemory = GlobalAlloc(GHND, sizeof(DROPFILES) + len + 1);

      if (!globalMemory) {
        return E_POINTER;
      }

      dropFiles = (DROPFILES*) GlobalLock(globalMemory);

      if (!dropFiles) {
        GlobalFree(globalMemory);
        return E_POINTER;
      }

      dropFiles->fNC = TRUE;
      dropFiles->fWide = FALSE; // Should probably be true for unicode
      dropFiles->pFiles = sizeof(DROPFILES);
      GetCursorPos(&(dropFiles->pt));

      char *dropFilePtr = (char *) &dropFiles[1];
      for (Vector<String>::size_type i = 0; i < files.size(); ++i) {
        String &file = files[i];

        len = (file.length() + 1);

        memcpy(dropFilePtr, file.c_str(), len);
        dropFilePtr += len;
      }

      GlobalUnlock(globalMemory);
      medium.hGlobal = globalMemory;

      if (!medium.hGlobal) {
        return S_OK;
      }

      CreateDropSource(&dropSource);
      CreateDataObject(&format, &medium, 2, &dataObject);

      // ReleaseCapture();
      dragDropResult = DoDragDrop(
        dataObject,
        dropSource,
        DROPEFFECT_COPY,
        &dropEffect
      );

      if (dragDropResult == DRAGDROP_S_CANCEL) {
        return S_OK;
      }

      if ((dropEffect & DROPEFFECT_COPY) == DROPEFFECT_COPY) {
        // @TODO(jwerle)
      }

      if ((dropEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE) {
        // @TODO(jwerle)
      }

      this->draggablePayload.clear();
      dropSource->Release();
      dataObject->Release();
      GlobalFree(globalMemory);

      return S_OK;
    };

    HRESULT STDMETHODCALLTYPE Drop (
      IDataObject *dataObject,
      DWORD keyState,
      POINTL dragPoint,
      DWORD *dragEffect
    ) {
      //
      // on windows pt.{x,y} are screen coordinates, to get the position of the mouse relative to the window
      // we need to subtract window.{left,top}.
      //

      auto position = POINT {0};
      auto parent = this->window->window;
      auto child = this->childWindow;
      auto point = POINT {0};
      auto rect = RECT {0};

      STGMEDIUM medium;
      FORMATETC format;
      HRESULT result;
      HDROP drop;
      int count;

      StringStream filesStringArray;

      GetClientRect(child, &rect);
      position = { rect.left, rect.top };

      MapWindowPoints(parent, GetParent(parent), (LPPOINT) &position, 2);
      point.x = dragPoint.x - position.x;
      point.y = dragPoint.y - position.y;

      format.dwAspect = DVASPECT_CONTENT;
      format.cfFormat = CF_HDROP;
      format.lindex = -1;
      format.tymed = TYMED_HGLOBAL;
      format.ptd = nullptr;

      if (
        SUCCEEDED(dataObject->QueryGetData(&format)) &&
        SUCCEEDED(dataObject->GetData(&format, &medium))
      ) {
        *dragEffect = DROPEFFECT_COPY;

        drop = (HDROP) GlobalLock(medium.hGlobal);
        count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);

        for (int i = 0; i < count; i++) {
          int size = DragQueryFile(drop, i, nullptr, 0);

          TCHAR* buf = new TCHAR[size + 1];
          DragQueryFile(drop, i, buf, size + 1);

          // append escaped file path with wrapped quotes ('"')
          filesStringArray
            << '"'
            << replace(String(buf), "\\\\", "\\\\")
            << '"';

          if (i < count - 1) {
            filesStringArray << ",";
          }

          delete[] buf;
        }

        this->window->eval(
          "(() => {                                                          \n"
          "  const value = {                                                 \n"
          "    files: [" + filesStringArray.str() + "],                      \n"
          "    x: " + std::to_string(point.x) + " / window.devicePixelRatio, \n"
          "    y: " + std::to_string(point.y) + " / window.devicePixelRatio  \n"
          "  };                                                              \n"
          "                                                                  \n"
          "  const dtail = JSON.stringify(value);                            \n"
          "  const event = new window.CustomEvent('data', { detail });       \n"
          "})();"
        );

      } else {
        format.cfFormat = CF_TEXT;
        if (
          SUCCEEDED(dataObject->QueryGetData(&format)) &&
          SUCCEEDED(dataObject->GetData(&format, &medium))
        ) {
          *dragEffect = DROPEFFECT_MOVE;
          for (auto &src : this->draggablePayload) {
            String json = (
              "{"
              "  \"src\": \"" + src + "\","
              "  \"x\":" + std::to_string(point.x) + ","
              "  \"y\":" + std::to_string(point.y) + ""
              "}"
            );

            this->window->eval(getEmitToRenderProcessJavaScript("drop", json));
          }

          String json = (
            "{"
            "  \"x\":" + std::to_string(point.x) + ","
            "  \"y\":" + std::to_string(point.y) + ""
            "}"
          );

          this->window->eval(getEmitToRenderProcessJavaScript("dragend", json));
        }
      }

      this->draggablePayload.clear();

      GlobalUnlock(medium.hGlobal);
      ReleaseStgMedium(&medium);

      return S_OK;
    };
  };

  Window::Window (SharedPointer<Core> core, const Window::Options& options)
    : core(core),
      options(options),
      bridge(core, IPC::Bridge::Options {
        options.userConfig,
        options.as<IPC::Preload::Options>()
      }),
      hotkey(this),
      dialog(this)
  {
    // this may be an "empty" path if not available
    static const auto edgeRuntimePath = FileResource::getMicrosoftEdgeRuntimePath();
    static auto app = App::sharedApplication();
    app->isReady = false;

    if (!edgeRuntimePath.empty()) {
      const auto string = convertWStringToString(edgeRuntimePath.string());
      const auto value = replace(string, "\\\\", "\\\\");
      // inject the `EDGE_RUNTIME_DIRECTORY` environment variable directly into
      // the userConfig so it is available as an env var in the webview runtime
      this->bridge.userConfig["env_EDGE_RUNTIME_DIRECTORY"] = value;
      debug("Microsoft Edge Runtime directory set to '%ls'", edgeRuntimePath.c_str());
    }

    auto userConfig = this->bridge.userConfig;
    // only the root window can handle "agent" tasks
    const bool isAgent = (
      userConfig["application_agent"] == "true" &&
      options.index == 0
    );

    if (isAgent) {
      this->window = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        userConfig["meta_bundle_identifier"].c_str(),
        userConfig["meta_title"].c_str(),
        WS_OVERLAPPEDWINDOW,
        100000,
        100000,
        options.width,
        options.height,
        nullptr,
        nullptr,
        app->hInstance,
        nullptr
      );
    } else {
      DWORD style = WS_THICKFRAME;

      if (options.frameless) {
        style |= WS_POPUP;
      } else {
        style |= WS_OVERLAPPED;
        // Windows does not have the ability to reposition the decorations
        // In this case, we can assume that the user will draw their own controls.
        if (options.titlebarStyle != "hidden" && options.titlebarStyle != "hiddenInset") {
          if (options.closable) {
            style |= WS_CAPTION | WS_SYSMENU;

            if (options.minimizable) {
              style |= WS_MINIMIZEBOX;
            }

            if (options.maximizable) {
              style |= WS_MAXIMIZEBOX;
            }
          }
        }
      }

      if (!options.resizable) {
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
      }

      this->window = CreateWindowEx(
        options.headless
          ? WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
          : WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        userConfig["meta_bundle_identifier"].c_str(),
        userConfig["meta_title"].c_str(),
        style,
        100000,
        100000,
        options.width,
        options.height,
        nullptr,
        nullptr,
        app->hInstance,
        nullptr
      );
    }

    auto webviewEnvironmentOptions = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    webviewEnvironmentOptions->put_AdditionalBrowserArguments(L"--enable-features=msWebView2EnableDraggableRegions");

    this->drop = std::make_shared<DragDrop>(this);
    this->bridge.navigateFunction = [this] (const auto url) {
      this->navigate(url);
    };

    this->bridge.evaluateJavaScriptFunction = [this] (const auto source) {
      this->eval(source);
    };

    this->bridge.client.preload = IPC::Preload::compile({
      .client = UniqueClient {
        .id = this->bridge.client.id,
        .index = this->bridge.client.index
      },
      .index = options.index,
      .conduit = this->core->conduit.port,
      .userScript = options.userScript
    });

    if (options.aspectRatio.size() > 0) {
      auto parts = split(options.aspectRatio, ':');
      double aspectRatio = 0;

      try {
        aspectRatio = std::stof(trim(parts[0])) / std::stof(trim(parts[1]));
      } catch (...) {
        debug("invalid aspect ratio");
      }

      if (aspectRatio > 0) {
        RECT rect;
        GetClientRect(this->window, &rect);
        // SetWindowAspectRatio(window, MAKELONG((long)(rect.bottom * aspectRatio), rect.bottom), nullptr);
      }
    }

    // in theory these allow you to do drop files in elevated mode
    // FIXME(@jwerle): `ChangeWindowMessageFilter` will be deprecated and potentially removed
    // but `ChangeWindowMessageFilterEx` doesn't seem to be available for linkage
    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
    //ChangeWindowMessageFilterEx(this->window, WM_DROPFILES, /* MSGFLT_ALLOW */ 1, nullptr);
    //ChangeWindowMessageFilterEx(this->window, WM_COPYDATA, /* MSGFLT_ALLOW*/ 1, nullptr);
    //ChangeWindowMessageFilterEx(this->window, 0x0049, /* MSGFLT_ALLOW */ 1, nullptr);

    UpdateWindow(this->window);
    ShowWindow(this->window, isAgent ? SW_HIDE : SW_SHOWNORMAL);

    // make this `Window` instance as `GWLP_USERDATA`
    SetWindowLongPtr(this->window, GWLP_USERDATA, (LONG_PTR) this);

    this->hotkey.init();
    this->bridge.init();

    static const auto APPDATA = Path(convertStringToWString(Env::get("APPDATA")));

    if (APPDATA.empty() || !fs::exists(APPDATA)) {
      throw std::runtime_error(
        "Environment is in an invalid state: Could not determine 'APPDATA' path"
      );
    }

    // XXX(@jwerle): is this path correct? maybe we should use the bundle identifier here
    // instead of the executable filename
    static const auto edgeRuntimeUserDataPath = ({
      wchar_t modulefile[MAX_PATH];
      GetModuleFileNameW(nullptr, modulefile, MAX_PATH);
      auto file = (fs::path { modulefile }).filename();
      auto filename = convertStringToWString(file.string());
      APPDATA / filename;
    });

    this->bridge.configureSchemeHandlers({
      .webview = webviewEnvironmentOptions
    });

    CreateCoreWebView2EnvironmentWithOptions(
      edgeRuntimePath.empty() ? nullptr : convertStringToWString(edgeRuntimePath.string()).c_str(),
      convertStringToWString(edgeRuntimeUserDataPath.string()).c_str(),
      webviewEnvironmentOptions.Get(),
      Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([=, this](
        HRESULT result,
        ICoreWebView2Environment* env
      ) mutable {
        return env->CreateCoreWebView2Controller(
          this->window,
          Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([=, this](
            HRESULT result,
            ICoreWebView2Controller* controller
          ) mutable {
            const auto bundleIdentifier = userConfig["meta_bundle_identifier"];

            if (result != S_OK) {
              return result;
            }

            if (controller == nullptr) {
              return E_HANDLE;
            }

            // configure the webview controller
            do {
              RECT bounds;
              GetClientRect(this->window, &bounds);
              this->controller = controller;
              this->controller->get_CoreWebView2(&this->webview);
              this->controller->put_Bounds(bounds);
              this->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
              this->controller->AddRef();
              this->bridge.configureWebView(this->webview);
            } while (0);

            // configure the webview settings
            do {
              ICoreWebView2Settings* settings = nullptr;
              ICoreWebView2Settings2* settings2 = nullptr;
              ICoreWebView2Settings3* settings3 = nullptr;
              ICoreWebView2Settings6* settings6 = nullptr;
              ICoreWebView2Settings9* settings9 = nullptr;

              const auto wantsDebugMode = this->options.debug || isDebugEnabled();

              this->webview->get_Settings(&settings);

              settings2 = reinterpret_cast<ICoreWebView2Settings2*>(settings);
              settings3 = reinterpret_cast<ICoreWebView2Settings3*>(settings);
              settings6 = reinterpret_cast<ICoreWebView2Settings6*>(settings);
              settings9 = reinterpret_cast<ICoreWebView2Settings9*>(settings);

              settings->put_IsScriptEnabled(true);
              settings->put_IsStatusBarEnabled(false);
              settings->put_IsWebMessageEnabled(true);
              settings->put_AreDevToolsEnabled(wantsDebugMode);
              settings->put_AreHostObjectsAllowed(true);
              settings->put_IsZoomControlEnabled(false);
              settings->put_IsBuiltInErrorPageEnabled(false);
              settings->put_AreDefaultContextMenusEnabled(true);
              settings->put_AreDefaultScriptDialogsEnabled(true);

              // TODO(@jwerle): set user agent for runtime
              // settings2->put_UserAgent();

              settings3->put_AreBrowserAcceleratorKeysEnabled(wantsDebugMode);

              settings6->put_IsPinchZoomEnabled(false);
              settings6->put_IsSwipeNavigationEnabled(
                this->bridge.userConfig["webview_navigator_enable_navigation_destures"] == "true"
              );

              settings9->put_IsNonClientRegionSupportEnabled(true);
            } while (0);

            // enumerate all child windows to re-register drag/drop
            EnumChildWindows(
              this->window,
              [](HWND handle, LPARAM param) -> BOOL {
                const auto length = GetWindowTextLengthW(handle);
                const auto pointer = GetWindowLongPtr(reinterpret_cast<HWND>(param), GWLP_USERDATA);
                auto window = reinterpret_cast<Window*>(pointer);

                if (length > 0) {
                  auto buffer = std::make_shared<wchar_t[]>(length + 1);
                  auto text = convertWStringToString(buffer.get());
                  GetWindowTextW(handle, buffer.get(), length + 1);

                  if (text.find("Chrome") != String::npos) {
                    RevokeDragDrop(handle);
                    RegisterDragDrop(handle, window->drop.get());
                    window->drop->childWindow = handle;
                  }
                }

                return true;
              },
              reinterpret_cast<LPARAM>(this->window)
            );

            // configure webview
            do {
              ICoreWebView2_22* webview22 = nullptr;
              ICoreWebView2_3* webview3 = reinterpret_cast<ICoreWebView2_3*>(this->webview);

              this->webview->QueryInterface(IID_PPV_ARGS(&webview22));
              this->webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

              if (webview22 != nullptr) {
                this->bridge.userConfig["env_COREWEBVIEW2_22_AVAILABLE"] = "true";

                webview22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                  L"*",
                  COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                  COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL
                );
              }

              webview3->SetVirtualHostNameToFolderMapping(
                convertStringToWString(bundleIdentifier).c_str(),
                FileResource::getResourcesPath().c_str(),
                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
              );
            } while (0);

            // configure the user script preload
            do {
              auto preloadUserScriptSource = IPC::Preload::compile({
                .features = IPC::Preload::Options::Features {
                  .useGlobalCommonJS = false,
                  .useGlobalNodeJS = false,
                  .useTestScript = false,
                  .useHTMLMarkup = false,
                  .useESM = false,
                  .useGlobalArgs = true
                },
                .client = UniqueClient {
                  .id = this->bridge.client.id,
                  .index = this->bridge.client.index
                },
                .index = this->options.index,
                .conduit = this->core->conduit.port,
                .userScript = this->options.userScript
              });

              this->webview->AddScriptToExecuteOnDocumentCreated(
                // Note that this may not do anything as preload goes out of scope before event fires
                // Consider using w->preloadJavascript, but apps work without this
                convertStringToWString(preloadUserScriptSource.str()).c_str(),
                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                  [&](HRESULT error, PCWSTR id) -> HRESULT { return S_OK; }
                ).Get()
              );
            } while (0);

            // configure webview permission request handler
            do {
              EventRegistrationToken token;
              this->webview->add_PermissionRequested(
                Microsoft::WRL::Callback<ICoreWebView2PermissionRequestedEventHandler>([=, this](
                  ICoreWebView2 *_,
                  ICoreWebView2PermissionRequestedEventArgs *args
                ) mutable {
                  COREWEBVIEW2_PERMISSION_KIND kind;
                  args->get_PermissionKind(&kind);

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_MICROPHONE)  {
                    if (
                      userConfig["permissions_allow_microphone"] == "false" ||
                      userConfig["permissions_allow_user_media"] == "false"
                    ) {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_CAMERA)  {
                    if (
                      userConfig["permissions_allow_camera"] == "false" ||
                      userConfig["permissions_allow_user_media"] == "false"
                    ) {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_GEOLOCATION)  {
                    if (userConfig["permissions_allow_geolocation"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_NOTIFICATIONS)  {
                    if (userConfig["permissions_allow_notifications"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_OTHER_SENSORS)  {
                    if (userConfig["permissions_allow_sensors"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ) {
                    if (userConfig["permissions_allow_clipboard"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_AUTOPLAY) {
                    if (userConfig["permissions_allow_autoplay"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  if (kind == COREWEBVIEW2_PERMISSION_KIND_LOCAL_FONTS) {
                    if (userConfig["permissions_allow_local_fonts"] == "false") {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);
                    } else {
                      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                    }
                  }

                  return S_OK;
                }).Get(),
                &token
              );
            } while (0);

            // configure webview callback for `window.open()`
            do {
              EventRegistrationToken token;
              this->webview->add_NewWindowRequested(
                Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                  [&](ICoreWebView2* webview, ICoreWebView2NewWindowRequestedEventArgs* args) {
                    // TODO(@jwerle): handle 'window.open()'
                    args->put_Handled(true);
                    return S_OK;
                  }
                ).Get(),
                &token
              );
            } while (0);

            // configure webview message handler
            do {
              EventRegistrationToken token;
              this->webview->add_WebMessageReceived(
                Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>([=, this](
                  ICoreWebView2* webview,
                  ICoreWebView2WebMessageReceivedEventArgs* args
                ) -> HRESULT {
                  if (this->onMessage == nullptr) {
                    return S_OK;
                  }

                  ICoreWebView2Environment12* environment12 = nullptr;
                  ICoreWebView2Environment* environment = nullptr;
                  ICoreWebView2_18* webview18 = nullptr;
                  ICoreWebView2_2* webview2 = nullptr;

                  LPWSTR string;
                  args->TryGetWebMessageAsString(&string);
                  const auto message = IPC::Message(convertWStringToString(string));
                  CoTaskMemFree(string);

                  this->webview->QueryInterface(IID_PPV_ARGS(&webview2));
                  this->webview->QueryInterface(IID_PPV_ARGS(&webview18));
                  webview2->get_Environment(&environment);
                  environment->QueryInterface(IID_PPV_ARGS(&environment12));

                  if (!this->bridge.route(message.str(), nullptr, 0)) {
                    onMessage(message.str());
                  }

                  return S_OK;
                }).Get(),
                &token
              );
            } while (0);

            // configure webview web resource requested callback
            do {
              EventRegistrationToken token;
              webview->add_WebResourceRequested(
                Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>([=, this](
                  ICoreWebView2* webview,
                  ICoreWebView2WebResourceRequestedEventArgs* args
                ) {
                  ICoreWebView2WebResourceRequest* platformRequest = nullptr;
                  ICoreWebView2Environment* env = nullptr;
                  ICoreWebView2Deferral* deferral = nullptr;

                  // get platform request and environment from event args
                  do {
                    ICoreWebView2_2* webview2 = nullptr;
                    if (webview->QueryInterface(IID_PPV_ARGS(&webview2)) != S_OK) {
                      return E_FAIL;
                    }

                    if (webview2->get_Environment(&env) != S_OK) {
                      return E_FAIL;
                    }

                    if (args->get_Request(&platformRequest) != S_OK) {
                      return E_FAIL;
                    }
                  } while (0);

                  auto request = IPC::SchemeHandlers::Request::Builder(
                    &this->bridge.schemeHandlers,
                    platformRequest,
                    env
                  );

                  // get and set HTTP method
                  do {
                    LPWSTR method;
                    platformRequest->get_Method(&method);
                    request.setMethod(convertWStringToString(method));
                  } while (0);

                  // iterator all HTTP headers and set them
                  do {
                    ICoreWebView2HttpRequestHeaders* headers = nullptr;
                    ComPtr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
                    BOOL hasCurrent = false;
                    BOOL hasNext = false;

                    if (platformRequest->get_Headers(&headers) == S_OK && headers->GetIterator(&iterator) == S_OK) {
                      while (SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent) {
                        LPWSTR name;
                        LPWSTR value;

                        if (iterator->GetCurrentHeader(&name, &value) != S_OK) {
                          break;
                        }

                        request.setHeader(convertWStringToString(name), convertWStringToString(value));
                        if (iterator->MoveNext(&hasNext) != S_OK || !hasNext) {
                          break;
                        }
                      }
                    }
                  } while (0);

                  // get request body
                  if (
                    request.request->method == "POST" ||
                    request.request->method == "PUT" ||
                    request.request->method == "PATCH"
                  ) {
                    IStream* content = nullptr;
                    if (platformRequest->get_Content(&content) == S_OK && content != nullptr) {
                      STATSTG stat;
                      content->Stat(&stat, 0);
                      size_t size = stat.cbSize.QuadPart;
                      if (size > 0) {
                        auto buffer = std::make_shared<char[]>(size);
                        if (content->Read(buffer.get(), size, nullptr) == S_OK) {
                          request.setBody(IPC::SchemeHandlers::Body {
                            size,
                            std::move(buffer)
                          });
                        }
                      }
                    }
                  }

                  auto req = request.build();
                  if (args->GetDeferral(&deferral) != S_OK) {
                    return E_FAIL;
                  }

                  const auto handled = this->bridge.schemeHandlers.handleRequest(req, [=](const auto& response) mutable {
                    args->put_Response(response.platformResponse);
                    deferral->Complete();
                  });

                  if (!handled) {
                    auto response = IPC::SchemeHandlers::Response(req, 404);
                    response.finish();
                    args->put_Response(response.platformResponse);
                    deferral->Complete();
                  }

                  return S_OK;
                }).Get(),
                &token
              );
            } while (0);

            // notify app is ready
            app->isReady = true;
            return S_OK;
          }).Get()
        );
      }).Get()
    );
  }

  Window::~Window () {
  }

  ScreenSize Window::getScreenSize () {
    return ScreenSize {
      .width = GetSystemMetrics(SM_CXFULLSCREEN),
      .height = GetSystemMetrics(SM_CYFULLSCREEN)
    };
  }

  void Window::about () {
    auto app = App::sharedApplication();
    auto text = String(
      this->bridge.userConfig["build_name"] + " " +
      "v" + this->bridge.userConfig["meta_version"] + "\n" +
      "Built with ssc v" + VERSION_FULL_STRING + "\n" +
      this->bridge.userConfig["meta_copyright"]
    );

    MSGBOXPARAMS mbp;
    mbp.cbSize = sizeof(MSGBOXPARAMS);
    mbp.hwndOwner = this->window;
    mbp.hInstance = app->hInstance;
    mbp.lpszText = text.c_str();
    mbp.lpszCaption = this->bridge.userConfig["build_name"].c_str();
    mbp.dwStyle = MB_USERICON;
    mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    mbp.lpfnMsgBoxCallback = nullptr;
    mbp.dwContextHelpId = 0;

    MessageBoxIndirect(&mbp);
  }

  void Window::kill () {
    if (this->controller != nullptr) this->controller->Close();
    if (this->window != nullptr) {
      if (menubar != nullptr) DestroyMenu(menubar);
      if (menutray != nullptr) DestroyMenu(menutray);
      DestroyWindow(this->window);
    }
  }

  void Window::showInspector () {
    webview->OpenDevToolsWindow();
  }

  void Window::exit (int code) {
    if (this->onExit != nullptr) {
      if (menubar != nullptr) DestroyMenu(menubar);
      if (menutray != nullptr) DestroyMenu(menutray);
      this->onExit(code);
    }
  }

  void Window::close (int code) {
    if (options.shouldExitApplicationOnClose) {
      this->exit(0);
      DestroyWindow(window);
    } else {
      this->hide();
    }
  }

  void Window::maximize () {
    ShowWindow(window, SW_MAXIMIZE);
  }

  void Window::minimize () {
    ShowWindow(window, SW_MINIMIZE);
  }

  void Window::restore () {
    ShowWindow(window, SW_RESTORE);
  }

  void Window::show () {
    static auto userConfig = getUserConfig();
    auto isAgent = userConfig.count("application_agent") != 0;

    if (isAgent && this->options.index == 0) return;

    if (this->options.headless == false) {
      ShowWindow(window, SW_SHOWNORMAL);
      UpdateWindow(window);

      BringWindowToTop(window);

      RECT r, r1;
      GetWindowRect(window, &r);
      GetWindowRect(GetDesktopWindow(), &r1);

      MoveWindow(window, (
        (r1.right-r1.left) - (r.right-r.left)) / 2,
        ((r1.bottom-r1.top) - (r.bottom-r.top)) / 2,
        (r.right-r.left),
        (r.bottom-r.top),
        0
      );

      RECT rc;
      rc.top = 0;
      rc.left = 0;
      rc.bottom = 0;
      rc.right = 0;
      InvalidateRect(this->window, &rc, true);
      DrawMenuBar(this->window);
      RedrawWindow(this->window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    }
  }

  void Window::hide () {
    ShowWindow(window, SW_HIDE);
    UpdateWindow(window);
    this->eval(getEmitToRenderProcessJavaScript("window-hidden", "{}"));
  }

  void Window::resize (HWND window) {
    if (controller == nullptr) {
      return;
    }

    RECT bounds;
    GetClientRect(window, &bounds);
    controller->put_Bounds(bounds);
  }

  void Window::eval (const String& source, const EvalCallback& callback) {
    auto app = App::sharedApplication();
    app->dispatch([=, this] {
      if (this->webview == nullptr) {
        return;
      }

      this->webview->ExecuteScript(
        convertStringToWString(source).c_str(),
        Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
          [=, this](HRESULT error, PCWSTR result) -> HRESULT {
            if (callback != nullptr) {
              if (error != S_OK) {
                // TODO(@jwerle): figure out how to get the error message here
                callback(JSON::Error("An unknown error occurred"));
                return error;
              }

              const auto string = convertWStringToString(result);
              if (string == "null" || string == "undefined") {
                callback(nullptr);
              } else if (string == "true") {
                callback(true);
              } else if (string == "false") {
                callback(false);
              } else {
                double number = 0.0f;

                try {
                  number = std::stod(string);
                } catch (...) {
                  callback(string);
                  return S_OK;
                }

                callback(number);
              }
            }

            return S_OK;
          }
        ).Get()
      );
    });
  }

  void Window::navigate (const String& url) {
    auto app = App::sharedApplication();
    auto index = std::to_string(this->options.index);

    app->dispatch([=, this] {
      EventRegistrationToken token;
      this->webview->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [=, this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            BOOL success;
            args->get_IsSuccess(&success);
            webview->remove_NavigationCompleted(token);
            return S_OK;
          })
        .Get(),
        &token
      );

      webview->Navigate(convertStringToWString(url).c_str());
    });
  }

  const String Window::getTitle () const {
    if (window != nullptr) {
      const auto size = GetWindowTextLength(window) + 1;
      LPTSTR text = new TCHAR[size]{0};
      if (text != nullptr) {
        GetWindowText(window, text, size);
        const auto title = convertWStringToString(text);
        delete [] text;
        return title;
      }
    }

    return "";
  }

  void Window::setTitle (const String& title) {
    SetWindowText(window, title.c_str());
  }

  Window::Size Window::getSize () {
    // 100 are the min width/height that can be returned. Keep defaults in case
    // the function call fail.
    UINT32 height = 100;
    UINT32 width = 100;
    RECT rect;

    // Make sure controller exists, and the call to get window bounds succeeds.
    if (controller != nullptr && controller->get_Bounds(&rect) >= 0) {
      this->size.height = rect.bottom - rect.top;
      this->size.width = rect.right - rect.left;
    }

    return this->size;
  }

  const Window::Size Window::getSize () const {
    return this->size;
  }

  void Window::setSize (int width, int height, int hints) {
    auto style = GetWindowLong(window, GWL_STYLE);

    if (hints == WINDOW_HINT_FIXED) {
      style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
      style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    SetWindowLong(window, GWL_STYLE, style);

    if (hints == WINDOW_HINT_MAX) {
      maximumSize.x = width;
      maximumSize.y = height;
    } else if (hints == WINDOW_HINT_MIN) {
      minimumSize.x = width;
      minimumSize.y = height;
    } else {
      RECT r;
      r.left = r.top = 0;
      r.right = width;
      r.bottom = height;

      AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
      SetWindowPos(
        window,
        nullptr,
        r.left,
        r.top,
        r.right - r.left,
        r.bottom - r.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
      );

      resize(window);
    }

    this->size.width = width;
    this->size.height = height;
  }

  void Window::setPosition (float x, float y) {
    RECT r;
    r.left = x;
    r.top = y;
    r.right = this->size.width;
    r.bottom = this->size.height;

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    SetWindowPos(
      window,
      nullptr,
      r.left,
      r.top,
      r.right - r.left,
      r.bottom - r.top,
      SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
    );

    resize(window);
    this->position.x = x;
    this->position.y = y;
  }

  void Window::setTrayMenu (const String& value) {
    return this->setMenu(value, true);
  }

  void Window::setSystemMenu (const String& value) {
    return this->setMenu(value, false);
  }

  void Window::setMenu (const String& menuSource, const bool& isTrayMenu) {
    auto app = App::sharedApplication();
    auto userConfig = this->options.userConfig;

    if (menuSource.empty()) {
      return;
    }

    NOTIFYICONDATA nid;

    if (isTrayMenu) {

      auto cwd = app->getcwd();
      auto trayIconPath = String("application_tray_icon");

      if (fs::exists(fs::path(cwd) / (trayIconPath + ".png"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".png")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpeg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpeg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".ico"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".ico")).string();
      } else {
        trayIconPath = "";
      }

      HICON icon;
      if (trayIconPath.size() > 0) {
        icon = (HICON) LoadImageA(
          nullptr,
          trayIconPath.c_str(),
          IMAGE_ICON,
          GetSystemMetrics(SM_CXSMICON),
          GetSystemMetrics(SM_CXSMICON),
          LR_LOADFROMFILE
        );
      } else {
        icon = LoadIcon(
          GetModuleHandle(nullptr),
          reinterpret_cast<LPCSTR>(MAKEINTRESOURCE(IDI_APPLICATION))
        );
      }

      menutray = CreatePopupMenu();
      nid.cbSize = sizeof(NOTIFYICONDATA);
      nid.hWnd = window;
      nid.uID = 1871369;
      nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

      nid.uCallbackMessage = WM_SOCKET_TRAY;
      nid.hIcon = icon;
    } else {
      menubar = GetMenu(window);
    }

    auto menus = split(menuSource, ';');
    int itemId = 0;

    for (auto m : menus) {
      auto menuSource = split(m, '\n');
      auto line = trim(menuSource[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];

      HMENU subMenu;
      if (!isTrayMenu) subMenu = CreateMenu();
      if (isTrayMenu) lstrcpy(nid.szTip, userConfig["meta_title"].c_str());

      if (isTrayMenu && menuSource.size() == 1) {
        auto menuParts = split(line, ':');
        auto title = menuParts[0];
        menuTrayMap[itemId] =  line;
        AppendMenuA(menutray, MF_STRING, itemId++, title.c_str());
      }

      for (int i = 1; i < menuSource.size(); i++) {
        auto line = trim(menuSource[i]);
        if (line.size() == 0) continue;

        if (line.empty()) {
          continue;
        }

        if (line.find("---") != -1) {
          if (isTrayMenu) {
            AppendMenuW(menutray, MF_SEPARATOR, 0, nullptr);
          } else {
            AppendMenuW(subMenu, MF_SEPARATOR, 0, nullptr);
          }
          continue;
        }

        auto parts = split(line, ':');
        auto title = parts[0];
        int mask = 0;
        String key = "";

        auto accelerators = split(parts[1], '+');
        auto accl = String("");

        key = trim(parts[1]) == "_" ? "" : trim(accelerators[0]);

        if (key.size() > 0) {
          bool isShift = String("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;
          accl = key;

          if (accelerators.size() > 1) {
            accl = String(trim(accelerators[1]) + "+" + key);
            accl = replace(accl, "CommandOrControl", "Ctrl");
            accl = replace(accl, "Command", "Ctrl");
            accl = replace(accl, "Control", "Ctrl");
            accl = replace(accl, "Super", "Meta");
          }

          if (isShift) {
            accl = String("Shift+" + accl);
          }
        }

        auto display = String(title + "\t" + accl);

        if (isTrayMenu) {
          AppendMenuA(menutray, MF_STRING, itemId, display.c_str());
          menuTrayMap[itemId] = String(title + ":" +(parts.size() > 1 ? parts[1] : ""));
        } else {
          AppendMenuA(subMenu, MF_STRING, itemId, display.c_str());
          menuMap[itemId] = String(title + "\t" + menuTitle);
        }

        itemId++;
      }

      if (!isTrayMenu) {
        AppendMenuA(menubar, MF_POPUP, (UINT_PTR) subMenu, menuTitle.c_str());
      }
    }

    if (isTrayMenu) {
      Shell_NotifyIcon(NIM_ADD, &nid);
    } else {
      MENUINFO Info;
      Info.cbSize = sizeof(Info);
      Info.fMask = MIM_BACKGROUND | MFT_OWNERDRAW;
      Info.hbrBack = CreateSolidBrush(RGB(0, 0, 0));
      SetMenuInfo(menubar, &Info);

      RECT rc;
      rc.top = 0;
      rc.left = 0;
      rc.bottom = 0;
      rc.right = 0;
      InvalidateRect(this->window, &rc, true);
      DrawMenuBar(this->window);
      RedrawWindow(this->window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // TODO
  }

  void Window::closeContextMenu() {
    // TODO
  }

  void Window::closeContextMenu (const String &seq) {
    // TODO
  }

  void Window::setContextMenu (const String& seq, const String& menuSource) {
    if (menuSource.empty()) {
      return;
    }

    const auto menuItems = split(menuSource, '\n');
    auto menu = CreatePopupMenu();
    Vector<String> lookup;
    int index = 1;

    lookup.push_back("");

    for (const auto& item : menuItems) {
      const auto pair = split(trim(item), ':');

      if (pair[0].find("---") != -1) {
        InsertMenu(menu, 0, MF_SEPARATOR, 0, nullptr);
      } else {
        lookup.push_back(pair[0]);
        InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, index++, pair[0].c_str());
      }
    }

    SetForegroundWindow(this->window);

    POINT point;
    GetCursorPos(&point);

    const auto selection = TrackPopupMenu(
      menu,
      TPM_RETURNCMD | TPM_NONOTIFY,
      point.x,
      point.y,
      0,
      this->window,
      nullptr
    );

    DestroyMenu(menu);

    if (selection != 0) {
      this->eval(getResolveMenuSelectionJavaScript(seq, lookup.at(selection), "contextMenu", "context"));
    }
  }

  void Window::setBackgroundColor (const String& rgba) {
    const auto parts = split(trim(replace(replace(rgba, "rgba(", ""), ")", "")), ',');
    int r = 0, g = 0, b = 0;

    if (parts.size() >= 3) {
      try { r = std::stoi(trim(parts[0])); }
      catch (...) {}

      try { g = std::stoi(trim(parts[1])); }
      catch (...) {}

      try { b = std::stoi(trim(parts[2])); }
      catch (...) {}

      return this->setBackgroundColor(r, g, b, 1.0);
    }
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    SetBkColor(GetDC(this->window), RGB(r, g, b));
  }

  String Window::getBackgroundColor () {
    auto color = GetBkColor(GetDC(this->window));
    if (color == CLR_INVALID) {
      return "";
    }

    const auto r = GetRValue(color);
    const auto g = GetGValue(color);
    const auto b = GetBValue(color);

    char string[100];

    snprintf(
      string,
      sizeof(string),
      "rgba(%d, %d, %d, %f)",
      r,
      g,
      b,
      1.0f
    );

    return string;
  }

  void Window::handleApplicationURL (const String& url) {
    JSON::Object json = JSON::Object::Entries {{
      "url", url
    }};

    this->bridge.emit("applicationurl", json.str());
  }
}
