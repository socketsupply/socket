#include "window.hh"

#ifndef CHECK_FAILURE
#define CHECK_FAILURE(...)
#endif

using namespace Microsoft::WRL;

namespace SSC {
  static inline void alert (const SSC::WString &ws) {
    MessageBoxA(nullptr, SSC::convertWStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const SSC::String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

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
    pFormatEtcOut->ptd = NULL;
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
    SSC::Vector<SSC::String> draggablePayload;
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
      *ppv = NULL;
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
      format.ptd      = NULL;
      format.dwAspect = DVASPECT_CONTENT;
      format.lindex   = -1;
      format.tymed    = TYMED_HGLOBAL;

      dataObject->QueryGetData(&format);
      dataObject->GetData(&format, &medium);

      list = (char *) GlobalLock(medium.hGlobal);

      this->draggablePayload.clear();

      if (list != 0) {
        draggablePayload = SSC::split(SSC::String(list), ';');


        SSC::String json = (
          "{"
          "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
          "  \"inbound\": true,"
          "  \"x\":" + std::to_string(point.x) + ","
          "  \"y\":" + std::to_string(point.y) + ""
          "}"
        );

        auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json);
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

      SSC::String json = (
        "{"
        "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
        "  \"inbound\": false,"
        "  \"x\":" + std::to_string(point.x) + ","
        "  \"y\":" + std::to_string(point.y) + ""
        "}"
      );

      auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json);
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

      SSC::Vector<SSC::String> files = this->draggablePayload;

      for (auto &file : files) {
        file = file.substr(12);
        len += file.size();
      }

      globalMemory = GlobalAlloc(GHND, sizeof(DROPFILES) + len + 1);

      if (!globalMemory) {
        return NULL;
      }

      dropFiles = (DROPFILES*) GlobalLock(globalMemory);

      if (!dropFiles) {
        GlobalFree(globalMemory);
        return NULL;
      }

      dropFiles->fNC = TRUE;
      dropFiles->fWide = FALSE; // Should probably be true for unicode
      dropFiles->pFiles = sizeof(DROPFILES);
      GetCursorPos(&(dropFiles->pt));

      char *dropFilePtr = (char *) &dropFiles[1];
      for (SSC::Vector<SSC::String>::size_type i = 0; i < files.size(); ++i) {
        SSC::String &file = files[i];

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

      SSC::StringStream filesStringArray;

      GetClientRect(child, &rect);
      position = { rect.left, rect.top };

      MapWindowPoints(parent, GetParent(parent), (LPPOINT) &position, 2);
      point.x = dragPoint.x - position.x;
      point.y = dragPoint.y - position.y;

      format.dwAspect = DVASPECT_CONTENT;
      format.cfFormat = CF_HDROP;
      format.lindex = -1;
      format.tymed = TYMED_HGLOBAL;
      format.ptd = NULL;

      if (
        SUCCEEDED(dataObject->QueryGetData(&format)) &&
        SUCCEEDED(dataObject->GetData(&format, &medium))
      ) {
        *dragEffect = DROPEFFECT_COPY;

        drop = (HDROP) GlobalLock(medium.hGlobal);
        count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);

        for (int i = 0; i < count; i++) {
          int size = DragQueryFile(drop, i, NULL, 0);

          TCHAR* buf = new TCHAR[size + 1];
          DragQueryFile(drop, i, buf, size + 1);

          // append escaped file path with wrapped quotes ('"')
          filesStringArray
            << '"'
            << SSC::replace(SSC::String(buf), "\\\\", "\\\\")
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
            SSC::String json = (
              "{"
              "  \"src\": \"" + src + "\","
              "  \"x\":" + std::to_string(point.x) + ","
              "  \"y\":" + std::to_string(point.y) + ""
              "}"
            );

            this->window->eval(SSC::getEmitToRenderProcessJavaScript("drop", json));
          }

          SSC::String json = (
            "{"
            "  \"x\":" + std::to_string(point.x) + ","
            "  \"y\":" + std::to_string(point.y) + ""
            "}"
          );

          this->window->eval(SSC::getEmitToRenderProcessJavaScript("dragend", json));
        }
      }

      this->draggablePayload.clear();

      GlobalUnlock(medium.hGlobal);
      ReleaseStgMedium(&medium);

      return S_OK;
    };
  };

  Window::Window (App& app, Window::Options opts)
    : app(app),
      opts(opts),
      hotkey(this)
  {
    static auto userConfig = SSC::getUserConfig();
    const bool isAgent = userConfig["application_agent"] == "true" && opts.index == 0;

    app.isReady = false;

    this->index = opts.index;
    if (isAgent && opts.index == 0) {
      window = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        userConfig["meta_bundle_identifier"].c_str(),
        userConfig["meta_title"].c_str(),
        WS_OVERLAPPEDWINDOW,
        100000,
        100000,
        opts.width,
        opts.height,
        NULL,
        NULL,
        app.hInstance,
        NULL
      );
    } else {
      DWORD style = WS_THICKFRAME;

      if (!opts.frameless) {
        style |= WS_OVERLAPPED;

        if (opts.titlebarStyle == "hidden" || opts.titlebarStyle == "hiddenInset") {
          // Windows does not have the ability to reposition the decorations
          // In this case, we can assume that the user will draw their own controls.
        } else if (opts.closable) {
          style |= WS_CAPTION | WS_SYSMENU;

          if (opts.minimizable) style |= WS_MINIMIZEBOX;
          if (opts.maximizable) style |= WS_MAXIMIZEBOX;
        }
      } else {
        style |= WS_POPUP;
      }

      window = CreateWindowEx(
        opts.headless
          ? WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE
          : WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        userConfig["meta_bundle_identifier"].c_str(),
        userConfig["meta_title"].c_str(),
        style,
        100000,
        100000,
        opts.width,
        opts.height,
        NULL,
        NULL,
        app.hInstance,
        NULL
      );
    }

    HRESULT initResult = OleInitialize(NULL);

    this->drop = new DragDrop(this);

    this->bridge = new IPC::Bridge(app.core, opts.userConfig);
    opts.clientId = this->bridge->id;

    this->hotkey.init(this->bridge);

    if (this->opts.aspectRatio.size() > 0) {
      auto parts = split(this->opts.aspectRatio, ':');
      double aspectRatio = 0;

      try {
        aspectRatio = std::stof(trim(parts[0])) / std::stof(trim(parts[1]));
      } catch (...) {
        debug("invalid aspect ratio");
      }

      if (aspectRatio > 0) {
        RECT rect;
        GetClientRect(window, &rect);
        // SetWindowAspectRatio(window, MAKELONG((long)(rect.bottom * aspectRatio), rect.bottom), NULL);
      }
    }

    this->bridge->router.dispatchFunction = [&app] (auto callback) {
      app.dispatch([callback] { callback(); });
    };

    this->bridge->router.evaluateJavaScriptFunction = [this] (auto js) {
      this->eval(js);
    };

    //
    // In theory these allow you to do drop files in elevated mode
    //
    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

    UpdateWindow(window);
    ShowWindow(window, isAgent ? SW_HIDE : SW_SHOWNORMAL);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) this);

    // this is something like "C:\\Users\\josep\\AppData\\Local\\Microsoft\\Edge SxS\\Application\\123.0.2386.0"
    auto EDGE_RUNTIME_DIRECTORY = convertStringToWString(trim(Env::get("SOCKET_EDGE_RUNTIME_DIRECTORY")));

    if (EDGE_RUNTIME_DIRECTORY.size() > 0 && fs::exists(EDGE_RUNTIME_DIRECTORY)) {
      usingCustomEdgeRuntimeDirectory = true;
      opts.userConfig["env_EDGE_RUNTIME_DIRECTORY"] = replace(convertWStringToString(EDGE_RUNTIME_DIRECTORY), "\\\\", "\\\\");
      debug("Using Edge Runtime Directory: %ls", EDGE_RUNTIME_DIRECTORY.c_str());
    } else {
      EDGE_RUNTIME_DIRECTORY = L"";
    }

    wchar_t modulefile[MAX_PATH];
    GetModuleFileNameW(NULL, modulefile, MAX_PATH);
    auto file = (fs::path { modulefile }).filename();
    auto filename = SSC::convertStringToWString(file.string());
    auto path = SSC::convertStringToWString(Env::get("APPDATA"));
    this->modulePath = fs::path(modulefile);

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(L"--enable-features=msWebView2EnableDraggableRegions");

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options4;
    HRESULT oeResult = options.As(&options4);
    if (oeResult != S_OK) {
      // UNREACHABLE - cannot continue
    }

    this->bridge->router.configureHandlers({
      options
    });

    auto init = [&, opts]() -> HRESULT {
      return CreateCoreWebView2EnvironmentWithOptions(
        EDGE_RUNTIME_DIRECTORY.size() > 0 ? EDGE_RUNTIME_DIRECTORY.c_str() : nullptr,
        (path + L"\\" + filename).c_str(),
        options.Get(),
        Microsoft::WRL::Callback<IEnvHandler>(
          [&, opts](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
            env->CreateCoreWebView2Controller(
              window,
              Microsoft::WRL::Callback<IConHandler>(
                [&, opts](HRESULT result, ICoreWebView2Controller* c) -> HRESULT {
                  static auto userConfig = SSC::getUserConfig();
                  if (c != nullptr) {
                    controller = c;
                    controller->get_CoreWebView2(&webview);

                    RECT bounds;
                    GetClientRect(window, &bounds);
                    controller->put_Bounds(bounds);
                    controller->AddRef();
                    controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
                  }

                  ICoreWebView2Settings* Settings;
                  webview->get_Settings(&Settings);
                  Settings->put_IsScriptEnabled(TRUE);
                  Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                  Settings->put_IsWebMessageEnabled(TRUE);
                  Settings->put_AreHostObjectsAllowed(TRUE);
                  Settings->put_IsStatusBarEnabled(FALSE);

                  Settings->put_AreDefaultContextMenusEnabled(TRUE);
                  if (opts.debug || isDebugEnabled()) {
                    Settings->put_AreDevToolsEnabled(TRUE);
                  } else {
                    Settings->put_AreDevToolsEnabled(FALSE);
                  }

                  Settings->put_IsBuiltInErrorPageEnabled(FALSE);
                  Settings->put_IsZoomControlEnabled(FALSE);

                  auto settings3 = (ICoreWebView2Settings3*) Settings;
                  if (!isDebugEnabled()) {
                    settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
                  }

                  auto settings6 = (ICoreWebView2Settings6*) Settings;
                  settings6->put_IsPinchZoomEnabled(FALSE);
                  settings6->put_IsSwipeNavigationEnabled(FALSE);

                  EnumChildWindows(window, [](HWND hWnd, LPARAM window) -> BOOL {
                    int l = GetWindowTextLengthW(hWnd);

                    if (l > 0) {
                      wchar_t* buf = new wchar_t[l+1];
                      GetWindowTextW(hWnd, buf, l+1);

                      if (SSC::convertWStringToString(buf).find("Chrome") != SSC::String::npos) {
                        RevokeDragDrop(hWnd);
                        Window* w = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)window, GWLP_USERDATA));
                        w->drop->childWindow = hWnd;
                        RegisterDragDrop(hWnd, w->drop);
                      }
                    }
                    return TRUE;
                  }, (LPARAM)window);

                  reinterpret_cast<ICoreWebView2_3*>(webview)->SetVirtualHostNameToFolderMapping(
                    convertStringToWString(userConfig["meta_bundle_identifier"]).c_str(),
                    this->modulePath.parent_path().c_str(),
                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
                  );

                  EventRegistrationToken tokenSchemaFilter;
                  webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

                  ICoreWebView2_22* webview22 = nullptr;
                  webview->QueryInterface(IID_PPV_ARGS(&webview22));

                  if (webview22 != nullptr) {
                    webview22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                      L"*",
                      COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                      COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL
                    );

                    debug("Configured CoreWebView2 (ICoreWebView2_22) request filter with all request source kinds");
                  }

                  webview->add_WebResourceRequested(
                    Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                      [&, opts](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* event) {
                        ICoreWebView2WebResourceRequest* platformRequest  = nullptr;
                        ICoreWebView2HttpRequestHeaders* headers = nullptr;
                        ICoreWebView2Environment* env = nullptr;
                        ICoreWebView2_2* webview2 = nullptr;


                        LPWSTR method;
                        LPWSTR uri;

                        webview->QueryInterface(IID_PPV_ARGS(&webview2));
                        webview2->get_Environment(&env);
                        event->get_Request(&platformRequest);

                        platformRequest->get_Headers(&headers);
                        platformRequest->get_Method(&method);
                        platformRequest->get_Uri(&uri);

                        auto request = IPC::SchemeHandlers::Request::Builder(this->bridge->router->schemeHandlers, platformRequest);

                        request.setMethod(convertWStringToString(method));

                        do {
                          ComPtr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
                          BOOL hasCurrent = FALSE;
                          CHECK_FAILURE(headers->GetIterator(&iterator));
                          while (SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent) {
                            LPWSTR name;
                            LPWSTR value;

                            if (iterator->GetCurrentHeader(&name, &value) == S_OK) {
                              request.setHeader(convertWStringToString(name), convertWStringToString(value));
                            }
                          }
                        } while (0);


                        String method;
                        String uri;

                        webview->QueryInterface(IID_PPV_ARGS(&webview2));
                        webview2->get_Environment(&env);
                        args->get_Request(&req);

                        req->get_Uri(&req_uri);
                        uri = convertWStringToString(req_uri);
                        CoTaskMemFree(req_uri);

                        req->get_Method(&req_method);
                        method = convertWStringToString(req_method);
                        CoTaskMemFree(req_method);

                        bool ipc_scheme = false;
                        bool socket_scheme = false;
                        bool handled = false;

                        if (uri.compare(0, 4, "ipc:") == 0) {
                          ipc_scheme = true;
                        } else if (uri.compare(0, 7, "socket:") == 0) {
                          socket_scheme = true;
                        } else {
                          return S_OK;
                        }

                        // Handle CORS preflight request.
                        if (method.compare("OPTIONS") == 0) {
                          ICoreWebView2WebResourceResponse* res = nullptr;
                          env->CreateWebResourceResponse(
                            nullptr,
                            204,
                            L"OK",
                            L"Connection: keep-alive\n"
                            L"Cache-Control: no-cache\n"
                            L"Access-Control-Allow-Headers: *\n"
                            L"Access-Control-Allow-Origin: *\n"
                            L"Access-Control-Allow-Methods: GET, POST, PUT, HEAD\n",
                            &res
                          );
                          args->put_Response(res);

                          return S_OK;
                        }


                        ICoreWebView2Deferral* deferral;
                        HRESULT hr = args->GetDeferral(&deferral);

                        char* body_ptr = nullptr;
                        size_t body_length = 0;

                        if (ipc_scheme) {
                          if (method.compare("POST") == 0 || method.compare("PUT") == 0) {
                            IStream* body_data;
                            DWORD actual;
                            HRESULT r;
                            auto msg = IPC::Message(uri);
                            msg.isHTTP = true;
                            // TODO(heapwolf): Make sure index and seq are set.
                            if (w->bridge->router.hasMappedBuffer(msg.index, msg.seq)) {
                              IPC::MessageBuffer buf = w->bridge->router.getMappedBuffer(msg.index, msg.seq);
                              ICoreWebView2SharedBuffer* shared_buf = buf.shared_buf;
                              size_t size = buf.size;
                              char* data = new char[size];
                              w->bridge->router.removeMappedBuffer(msg.index, msg.seq);
                              shared_buf->OpenStream(&body_data);
                              r = body_data->Read(data, size, &actual);
                              if (r == S_OK || r == S_FALSE) {
                                body_ptr = data;
                                body_length = actual;
                              } else {
                                delete[] data;
                              }
                              shared_buf->Close();
                            }
                          }

                          handled = w->bridge->route(uri, body_ptr, body_length, [&, args, deferral, env, body_ptr](auto result) {
                            String headers;
                            char* body;
                            size_t length;

                            if (body_ptr != nullptr) {
                              delete[] body_ptr;
                            }

                            if (result.post.body != nullptr) {
                              length = result.post.length;
                              body = new char[length];
                              memcpy(body, result.post.body, length);
                              headers = "Content-Type: application/octet-stream\n";
                            } else {
                              length = result.str().size();
                              body = new char[length];
                              memcpy(body, result.str().c_str(), length);
                              headers = "Content-Type: application/json\n";
                            }

                            headers += "Connection: keep-alive\n";
                            headers += "Cache-Control: no-cache\n";
                            headers += "Access-Control-Allow-Headers: *\n";
                            headers += "Access-Control-Allow-Origin: *\n";
                            headers += "Content-Length: ";
                            headers += std::to_string(length);
                            headers += "\n";

                            // Completing the response in the call to dispatch because the
                            // put_Response() must be called from the same thread that made
                            // the request. This assumes that the request was made from the
                            // main thread, since that's where dispatch() will call its cb.
                            app.dispatch([&, body, length, headers, args, deferral, env] {
                              ICoreWebView2WebResourceResponse* res = nullptr;
                              IStream* bytes = SHCreateMemStream((const BYTE*)body, length);
                              env->CreateWebResourceResponse(
                                bytes,
                                200,
                                L"OK",
                                convertStringToWString(headers).c_str(),
                                &res
                              );
                              args->put_Response(res);
                              deferral->Complete();
                              delete[] body;
                            });
                          });
                        }

                        if (socket_scheme) {
                          if (method.compare("GET") == 0 || method.compare("HEAD") == 0) {
                            if (uri.starts_with("socket:///")) {
                              uri = uri.substr(10);
                            } else if (uri.starts_with("socket://")) {
                              uri = uri.substr(9);
                            } else if (uri.starts_with("socket:")) {
                              uri = uri.substr(7);
                            }

                            auto path = String(
                              uri.starts_with(bundleIdentifier)
                                ? uri.substr(bundleIdentifier.size())
                                : "socket/" + uri
                            );

                            const auto parts = split(path, '?');
                            const auto query = parts.size() > 1 ? String("?") + parts[1] : "";
                            path = parts[0];

                            auto ext = fs::path(path).extension().string();

                            if (ext.size() > 0 && !ext.starts_with(".")) {
                              ext = "." + ext;
                            }

                            if (!uri.starts_with(bundleIdentifier)) {
                              if (path.ends_with("/")) {
                                path = path.substr(0, path.size() - 1);
                              }

                              if (ext.size() == 0 && !path.ends_with(".js")) {
                                path += ".js";
                              }

                              if (path == "/") {
                                uri = "socket://" + bundleIdentifier + "/";
                              } else {
                                uri = "socket://" + bundleIdentifier + "/" + path;
                              }

                              String headers;

                              auto moduleUri = replace(uri, "\\\\", "/");
                              auto moduleSource = trim(tmpl(
                                moduleTemplate,
                                Map { {"url", String(moduleUri)} }
                              ));

                              auto length = moduleSource.size();

                              headers = "Content-Type: text/javascript\n";
                              headers += "Cache-Control: no-cache\n";
                              headers += "Connection: keep-alive\n";
                              headers += "Access-Control-Allow-Headers: *\n";
                              headers += "Access-Control-Allow-Origin: *\n";
                              headers += "Content-Length: ";
                              headers += std::to_string(length);
                              headers += "\n";
                              headers += userConfig["webview_headers"];

                              handled = true;

                              if (method.compare("HEAD") == 0) {
                                ICoreWebView2WebResourceResponse* res = nullptr;
                                env->CreateWebResourceResponse(
                                  nullptr,
                                  200,
                                  L"OK",
                                  convertStringToWString(headers).c_str(),
                                  &res
                                );
                                args->put_Response(res);
                                deferral->Complete();
                              } else {
                                auto body = new char[length];
                                memcpy(body, moduleSource.c_str(), length);

                                app.dispatch([&, body, length, headers, args, deferral, env] {
                                  ICoreWebView2WebResourceResponse* res = nullptr;
                                  IStream* bytes = SHCreateMemStream((const BYTE*)body, length);
                                  env->CreateWebResourceResponse(
                                    bytes,
                                    200,
                                    L"OK",
                                    convertStringToWString(headers).c_str(),
                                    &res
                                  );
                                  args->put_Response(res);
                                  deferral->Complete();
                                  delete[] body;
                                });
                              }
                            } else {
                              if (path.ends_with("//")) {
                                path = path.substr(0, path.size() - 2);
                              }

                              auto parsedPath = IPC::Router::parseURLComponents(path);
                              auto rootPath = this->modulePath.parent_path();
                              auto resolved = IPC::Router::resolveURLPathForWebView(parsedPath.path, rootPath.string());
                              auto mount = IPC::Router::resolveNavigatorMountForWebView(parsedPath.path);
                              path = resolved.path;

                              if (mount.path.size() > 0) {
                                if (mount.resolution.redirect) {
                                  auto redirectURL = mount.resolution.path;
                                  if (parsedPath.queryString.size() > 0) {
                                    redirectURL += "?" + parsedPath.queryString;
                                  }

                                  if (parsedPath.fragment.size() > 0) {
                                    redirectURL += "#" + parsedPath.fragment;
                                  }

                                  ICoreWebView2WebResourceResponse* res = nullptr;
                                  auto contentLocation = replace(redirectURL, "socket://" + bundleIdentifier, "");
                                  env->CreateWebResourceResponse(
                                    nullptr,
                                    301,
                                    L"Moved Permanently",
                                    WString(
                                      convertStringToWString("Location: ") + convertStringToWString(redirectURL) + L"\n" +
                                      convertStringToWString("Content-Location: ") + convertStringToWString(contentLocation) + L"\n"
                                      ).c_str(),
                                    &res
                                  );

                                  args->put_Response(res);
                                  deferral->Complete();
                                  return S_OK;
                                }
                              } else if (path.size() == 0 && userConfig.contains("webview_default_index")) {
                                path = userConfig["webview_default_index"];
                              } else if (resolved.redirect) {
                                auto redirectURL = resolved.path;
                                if (parsedPath.queryString.size() > 0) {
                                  redirectURL += "?" + parsedPath.queryString;
                                }

                                if (parsedPath.fragment.size() > 0) {
                                  redirectURL += "#" + parsedPath.fragment;
                                }

                                auto contentLocation = replace(redirectURL, "socket://" + bundleIdentifier, "");
                                ICoreWebView2WebResourceResponse* res = nullptr;
                                env->CreateWebResourceResponse(
                                  nullptr,
                                  301,
                                  L"Moved Permanently",
                                  WString(
                                    convertStringToWString("Location: ") + convertStringToWString(redirectURL) + L"\n" +
                                    convertStringToWString("Content-Location: ") + convertStringToWString(contentLocation) + L"\n"
                                    ).c_str(),
                                  &res
                                  );

                                args->put_Response(res);
                                deferral->Complete();
                                return S_OK;
                              }

                              if (mount.path.size() > 0) {
                                path = mount.path;
                              } else if (path.size() > 0) {
                                path = fs::absolute(rootPath / path.substr(1)).string();
                              }

                              LARGE_INTEGER fileSize;
                              auto handle = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
                              auto getSizeResult = GetFileSizeEx(handle, &fileSize);

                              if (handle) {
                                CloseHandle(handle);
                              }

                              if (getSizeResult) {
                                handled = true;
                                app.dispatch([&, path, args, deferral, env] {
                                  ICoreWebView2WebResourceResponse* res = nullptr;
                                  LPWSTR mimeType = (wchar_t*) L"application/octet-stream";
                                  IStream* stream = nullptr;
                                  String headers = "";

                                  if (path.ends_with(".js") || path.ends_with(".mjs") || path.ends_with(".cjs")) {
                                    mimeType = (wchar_t*) L"text/javascript";
                                  } else if (path.ends_with(".wasm")) {
                                    mimeType = (wchar_t*) L"application/wasm";
                                  } else if (path.ends_with(".ts")) {
                                    mimeType = (wchar_t*) L"application/typescript";
                                  } else if (path.ends_with(".html")) {
                                    mimeType = (wchar_t*) L"text/html";
                                  } else if (path.ends_with(".css")) {
                                    mimeType = (wchar_t*) L"text/css";
                                  } else if (path.ends_with(".png")) {
                                    mimeType = (wchar_t*) L"image/png";
                                  } else if (path.ends_with(".jpg") || path.ends_with(".jpeg")) {
                                    mimeType = (wchar_t*) L"image/jpeg";
                                  } else if (path.ends_with(".json")) {
                                    mimeType = (wchar_t*) L"application/json";
                                  } else if (path.ends_with(".jsonld")) {
                                    mimeType = (wchar_t*) L"application/ld+json";
                                  } else if (path.ends_with(".opus")) {
                                    mimeType = (wchar_t*) L"audio/opus";
                                  } else if (path.ends_with(".oga")) {
                                    mimeType = (wchar_t*) L"audio/ogg";
                                  } else if (path.ends_with(".mp3")) {
                                    mimeType = (wchar_t*) L"audio/mp3";
                                  } else if (path.ends_with(".mp4")) {
                                    mimeType = (wchar_t*) L"video/mp4";
                                  } else if (path.ends_with(".mpeg")) {
                                    mimeType = (wchar_t*) L"video/mpeg";
                                  } else if (path.ends_with(".ogv")) {
                                    mimeType = (wchar_t*) L"video/ogg";
                                  } else {
                                    FindMimeFromData(0, convertStringToWString(path).c_str(), 0, 0, 0, 0, &mimeType, 0);
                                  }

                                  headers = "Content-Type: ";
                                  headers += convertWStringToString(mimeType) + "\n";
                                  headers += "Connection: keep-alive\n";
                                  headers += "Cache-Control: no-cache\n";
                                  headers += "Access-Control-Allow-Headers: *\n";
                                  headers += "Access-Control-Allow-Origin: *\n";
                                  headers += "Content-Length: ";
                                  headers += std::to_string(fileSize.QuadPart);
                                  headers += "\n";
                                  headers += userConfig["webview_headers"];

                                  if (SHCreateStreamOnFileA(path.c_str(), STGM_READ, &stream) == S_OK) {
                                    env->CreateWebResourceResponse(
                                      stream,
                                      200,
                                      L"OK",
                                      convertStringToWString(headers).c_str(),
                                      &res
                                    );
                                  } else {
                                    env->CreateWebResourceResponse(
                                      nullptr,
                                      404,
                                      L"Not Found",
                                      L"Access-Control-Allow-Origin: *",
                                      &res
                                    );
                                  }
                                  args->put_Response(res);
                                  deferral->Complete();
                                });
                              }
                            }
                          }
                        }

                        if (!handled) {
                          ICoreWebView2WebResourceResponse* res = nullptr;
                          env->CreateWebResourceResponse(
                            nullptr,
                            404,
                            L"Not Found",
                            L"Access-Control-Allow-Origin: *",
                            &res
                          );
                          args->put_Response(res);
                          deferral->Complete();
                        }

                        return S_OK;
                      }
                    ).Get(),
                    &tokenSchemaFilter
                  );

                  EventRegistrationToken tokenNewWindow;

                  webview->add_NewWindowRequested(
                    Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                      [&](ICoreWebView2* wv, ICoreWebView2NewWindowRequestedEventArgs* e) {
                        // TODO(heapwolf): Called when window.open() is called in JS, but the new
                        // window won't have all the setup and request interception. This setup should
                        // be moved to another location where it can be run for any new window. Right
                        // now ipc won't work for any new window.
                        e->put_Handled(true);
                        return S_OK;
                      }
                    ).Get(),
                    &tokenNewWindow
                  );

                  webview->QueryInterface(IID_PPV_ARGS(&webview22));
                  this->bridge->userConfig["env_COREWEBVIEW2_22_AVAILABLE"] = webview22 != nullptr ? "true" : "";
                  this->bridge->preload = createPreload(opts, {
                    .module = true,
                    .wrap = true,
                    .userScript = opts.userScript
                  });

                  EventRegistrationToken tokenMessage;

                  webview->add_WebMessageReceived(
                    Microsoft::WRL::Callback<IRecHandler>([&](ICoreWebView2* webview, IArgs* args) -> HRESULT {
                      LPWSTR messageRaw;
                      args->TryGetWebMessageAsString(&messageRaw);                      
                      SSC::WString message_w(messageRaw);
                      CoTaskMemFree(messageRaw);
                      if (onMessage != nullptr) {
                        SSC::String message = SSC::convertWStringToString(message_w);
                        auto msg = IPC::Message{message};
                        Window* w = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)window, GWLP_USERDATA));
                        ICoreWebView2_2* webview2 = nullptr;
                        ICoreWebView2Environment* env = nullptr;
                        ICoreWebView2_18* webView18 = nullptr;
                        ICoreWebView2Environment12* environment = nullptr;

                        webview->QueryInterface(IID_PPV_ARGS(&webview2));
                        webview2->get_Environment(&env);
                        env->QueryInterface(IID_PPV_ARGS(&environment));

                        webview->QueryInterface(IID_PPV_ARGS(&webView18));

                        // this should only come from `postMessage()`
                        if (msg.name == "buffer.create") {
                          auto seq = msg.seq;
                          auto size = std::stoull(msg.get("size", "0"));
                          auto index = msg.index;
                          ICoreWebView2SharedBuffer* sharedBuffer = nullptr;
                          // TODO(heapwolf): What to do if creation fails, or size == 0?
                          HRESULT cshr = environment->CreateSharedBuffer(size, &sharedBuffer);
                          String additionalData = "{\"seq\":\"";
                          additionalData += seq;
                          additionalData += "\",\"index\":";
                          additionalData += std::to_string(index);
                          additionalData += "}";
                          cshr = webView18->PostSharedBufferToScript(
                            sharedBuffer,
                            COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_WRITE,
                            convertStringToWString(additionalData).c_str()
                          );
                          IPC::MessageBuffer msg_buf(sharedBuffer, size);
                          // TODO(heapwolf): This will leak memory if the buffer is created and
                          // placed on the map then never removed. Since there's no Window cleanup
                          // that will remove unused buffers when the window is closed.
                          w->bridge->router.setMappedBuffer(index, seq, msg_buf);
                          return S_OK;
                        }

                        if (!w->bridge->route(message, nullptr, 0)) {
                          onMessage(message);
                        }
                      }

                      return S_OK;
                    }).Get(),
                    &tokenMessage
                  );

                  EventRegistrationToken tokenPermissionRequested;
                  webview->add_PermissionRequested(
                    Microsoft::WRL::Callback<ICoreWebView2PermissionRequestedEventHandler>([&](
                      ICoreWebView2 *webview,
                      ICoreWebView2PermissionRequestedEventArgs *args
                    ) -> HRESULT {
                      static auto userConfig = SSC::getUserConfig();
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
                    &tokenPermissionRequested
                  );

                  app.isReady = true;

                  return S_OK;
                }
              ).Get()
            );

            return S_OK;
          }
        ).Get()
      );
    };

    auto res = init();

    if (!SUCCEEDED(res)) {
      std::cerr << "Webview2 failed to initialize: " << std::to_string(res) << std::endl;
    }
  }

  Window::~Window () {
    delete this->drop;
    delete this->bridge;
  }

  ScreenSize Window::getScreenSize () {
    return ScreenSize {
      .height = GetSystemMetrics(SM_CYFULLSCREEN),
      .width = GetSystemMetrics(SM_CXFULLSCREEN)
    };
  }

  void Window::about () {
    auto text = SSC::String(
      app.userConfig["build_name"] + " " +
      "v" + app.userConfig["meta_version"] + "\n" +
      "Built with ssc v" + SSC::VERSION_FULL_STRING + "\n" +
      app.userConfig["meta_copyright"]
    );

    MSGBOXPARAMS mbp;
    mbp.cbSize = sizeof(MSGBOXPARAMS);
    mbp.hwndOwner = window;
    mbp.hInstance = app.hInstance;
    mbp.lpszText = text.c_str();
    mbp.lpszCaption = app.userConfig["build_name"].c_str();
    mbp.dwStyle = MB_USERICON;
    mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    mbp.lpfnMsgBoxCallback = NULL;
    mbp.dwContextHelpId = 0;

    MessageBoxIndirect(&mbp);
  }

  void Window::kill () {
    if (this->controller != nullptr) this->controller->Close();
    if (this->window != nullptr) {
      if (menubar != NULL) DestroyMenu(menubar);
      if (menutray != NULL) DestroyMenu(menutray);
      DestroyWindow(this->window);
    }
  }

  void Window::showInspector () {
    webview->OpenDevToolsWindow();
  }

  void Window::exit (int code) {
    if (this->onExit != nullptr) {
      std::cerr << "WARNING: Window#" << index << " exiting with code " << code << std::endl;
      if (menubar != NULL) DestroyMenu(menubar);
      if (menutray != NULL) DestroyMenu(menutray);
      this->onExit(code);
    }
    else {
      std::cerr << "WARNING: Window#" << index << " window->onExit is null in Window::exit()" << std::endl;
    }
  }

  void Window::close (int code) {
    if (opts.shouldExitApplicationOnClose) {
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
    static auto userConfig = SSC::getUserConfig();
    auto isAgent = userConfig.count("application_agent") != 0;

    if (isAgent && this->opts.index == 0) return;

    if (this->opts.headless == false) {
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
      RedrawWindow(this->window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
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

  void Window::eval (const SSC::String& s) {
    app.dispatch([&, this, s] {
      if (this->webview == nullptr) {
        return;
      }

      this->webview->ExecuteScript(
        SSC::convertStringToWString(s).c_str(),
        nullptr
      );
    });
  }

  void Window::navigate (const String& url) {
    return this->navigate("", url);
  }

  void Window::navigate (const SSC::String& seq, const SSC::String& value) {
    auto index = std::to_string(this->opts.index);

    app.dispatch([&, this, seq, value, index] {
      EventRegistrationToken token;
      this->webview->add_NavigationCompleted(
        Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [&, this, seq, index, token](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            SSC::String state = "1";

            BOOL success;
            args->get_IsSuccess(&success);

            if (success) {
              state = "0";
            }

            this->resolvePromise(seq, state, index);
            webview->remove_NavigationCompleted(token);

            return S_OK;
          })
        .Get(),
        &token
      );

      webview->Navigate(SSC::convertStringToWString(value).c_str());
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

  void Window::setTitle (const SSC::String& title) {
    SetWindowText(window, title.c_str());
  }

  ScreenSize Window::getSize () {
    // 100 are the min width/height that can be returned. Keep defaults in case
    // the function call fail.
    UINT32 height = 100;
    UINT32 width = 100;
    RECT rect;

    // Make sure controller exists, and the call to get window bounds succeeds.
    if (controller != nullptr && controller->get_Bounds(&rect) >= 0) {
      this->height = rect.bottom - rect.top;
      this->width = rect.right - rect.left;
    }

    return { static_cast<int>(this->height), static_cast<int>(this->width) };
  }

  ScreenSize Window::getSize () const {
    return { static_cast<int>(this->height), static_cast<int>(this->width) };
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
        NULL,
        r.left,
        r.top,
        r.right - r.left,
        r.bottom - r.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
      );

      resize(window);
    }

    this->width = width;
    this->height = height;
  }

  void Window::setPosition (float x, float y) {
    RECT r;
    r.left = x;
    r.top = y;
    r.right = this->width;
    r.bottom = this->height;

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    SetWindowPos(
      window,
      NULL,
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

  void Window::setTrayMenu (const SSC::String& seq, const SSC::String& value) {
    setMenu(seq, value, true);
  }

  void Window::setSystemMenu (const SSC::String& seq, const SSC::String& value) {
    setMenu(seq, value, false);
  }

  void Window::setMenu (const SSC::String& seq, const SSC::String& menuSource, const bool& isTrayMenu) {
    static auto userConfig = SSC::getUserConfig();
    if (menuSource.empty()) return void(0);

    NOTIFYICONDATA nid;

    if (isTrayMenu) {
      static auto app = App::sharedApplication();

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
          NULL,
          trayIconPath.c_str(),
          IMAGE_ICON,
          GetSystemMetrics(SM_CXSMICON),
          GetSystemMetrics(SM_CXSMICON),
          LR_LOADFROMFILE
        );
      } else {
        icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APPLICATION));
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
            AppendMenuW(menutray, MF_SEPARATOR, 0, NULL);
          } else {
            AppendMenuW(subMenu, MF_SEPARATOR, 0, NULL);
          }
          continue;
        }

        auto parts = split(line, ':');
        auto title = parts[0];
        int mask = 0;
        SSC::String key = "";

        auto accelerators = split(parts[1], '+');
        auto accl = SSC::String("");

        key = trim(parts[1]) == "_" ? "" : trim(accelerators[0]);

        if (key.size() > 0) {
          bool isShift = SSC::String("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;
          accl = key;

          if (accelerators.size() > 1) {
            accl = SSC::String(trim(accelerators[1]) + "+" + key);
            accl = replace(accl, "CommandOrControl", "Ctrl");
            accl = replace(accl, "Command", "Ctrl");
            accl = replace(accl, "Control", "Ctrl");
            accl = replace(accl, "Super", "Meta");
          }

          if (isShift) {
            accl = SSC::String("Shift+" + accl);
          }
        }

        auto display = SSC::String(title + "\t" + accl);

        if (isTrayMenu) {
          AppendMenuA(menutray, MF_STRING, itemId, display.c_str());
          menuTrayMap[itemId] = SSC::String(title + ":" +(parts.size() > 1 ? parts[1] : ""));
        } else {
          AppendMenuA(subMenu, MF_STRING, itemId, display.c_str());
          menuMap[itemId] = SSC::String(title + "\t" + menuTitle);
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
      RedrawWindow(this->window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // @TODO(): provide impl
  }

  void Window::closeContextMenu() {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu(const SSC::String &seq) {
    // @TODO(jwerle)
  }

  void Window::setContextMenu (const SSC::String& seq, const SSC::String& menuSource) {
    if (menuSource.empty()) return void(0);

    HMENU hPopupMenu = CreatePopupMenu();

    auto menuItems = split(menuSource, '\n');
    int index = 1;
    std::vector<SSC::String> lookup;
    lookup.push_back("");

    for (auto item : menuItems) {
      auto pair = split(trim(item), ':');
      auto key = SSC::String("");

      if (pair.size() > 1) {
        key = pair[1];
      }

      if (pair[0].find("---") != -1) {
        InsertMenu(hPopupMenu, 0, MF_SEPARATOR, 0, NULL);
      } else {
        lookup.push_back(pair[0]);
        InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, index++, pair[0].c_str());
      }
    }

    SetForegroundWindow(window);

    POINT p;
    GetCursorPos(&p);

    auto selection = TrackPopupMenu(
      hPopupMenu,
      TPM_RETURNCMD | TPM_NONOTIFY,
      p.x,
      p.y,
      0,
      window,
      nullptr
    );

    DestroyMenu(hPopupMenu);
    if (selection == 0) return;
    this->eval(getResolveMenuSelectionJavaScript(seq, lookup.at(selection), "contextMenu", "context"));
  }

  void Window::setBackgroundColor (const String& rgba) {

  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    SetBkColor(GetDC(window), RGB(r, g, b));
    app.wcex.hbrBackground = CreateSolidBrush(RGB(r, g, b));
  }

  String Window::getBackgroundColor () {
    LOGBRUSH lb;
    GetObject(app.wcex.hbrBackground, sizeof(LOGBRUSH), &lb);

    int r = GetRValue(lb.lbColor);
    int g = GetGValue(lb.lbColor);
    int b = GetBValue(lb.lbColor);

    std::stringstream ss;
    ss << "R:" << r << ", G:" << g << ", B:" << b;
    return ss.str();
  }

  // message is defined in WinUser.h
  // https://raw.githubusercontent.com/tpn/winsdk-10/master/Include/10.0.10240.0/um/WinUser.h
  LRESULT CALLBACK Window::WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
  ) {
    static auto app = SSC::App::sharedApplication();
    Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (message == WM_COPYDATA) {
      auto copyData = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
      message = (UINT) copyData->dwData;
      wParam = (WPARAM) copyData->cbData;
      lParam = (LPARAM) copyData->lpData;
    }

    switch (message) {
      case WM_SIZE: {
        if (w == nullptr || w->webview == nullptr) {
          break;
        }

        RECT bounds;
        GetClientRect(hWnd, &bounds);
        w->controller->put_Bounds(bounds);
        break;
      }

      case WM_SOCKET_TRAY: {
        static auto userConfig = SSC::getUserConfig();
        auto isAgent = userConfig.count("tray_icon") != 0;

        if (lParam == WM_LBUTTONDOWN) {
          SetForegroundWindow(hWnd);
          if (isAgent) {
            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(w->menutray, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
          }

          PostMessage(hWnd, WM_NULL, 0, 0);

          // broadcast an event to all the windows that the tray icon was clicked
          if (app != nullptr && app->windowManager != nullptr) {
            for (auto window : app->windowManager->windows) {
              if (window != nullptr) {
                window->bridge->router.emit("tray", "true");
              }
            }
          }
        }
        // fall through to WM_COMMAND!!
      }

      case WM_COMMAND: {
        if (w == nullptr) break;

        if (w->menuMap.contains(wParam)) {
          String meta(w->menuMap[wParam]);
          auto parts = split(meta, '\t');

          if (parts.size() > 1) {
            auto title = parts[0];
            auto parent = parts[1];

            if (String(title).find("About") == 0) {
              w->about();
              break;
            }

            if (String(title).find("Quit") == 0) {
              w->exit(0);
              break;
            }

            w->eval(getResolveMenuSelectionJavaScript("0", title, parent, "system"));
          }
        } else if (w->menuTrayMap.contains(wParam)) {
          String meta(w->menuTrayMap[wParam]);
          auto parts = split(meta, ':');

          if (parts.size() > 0) {
            auto title = trim(parts[0]);
            auto tag = parts.size() > 1 ? trim(parts[1]) : "";
            w->eval(getResolveMenuSelectionJavaScript("0", title, tag, "tray"));
          }
        }

        break;
      }

      case WM_SETTINGCHANGE: {
        // TODO(heapwolf): Dark mode
        break;
      }

      case WM_CREATE: {
        // TODO(heapwolf): Dark mode
        SetWindowTheme(hWnd, L"Explorer", NULL);
        SetMenu(hWnd, CreateMenu());
        break;
      }

      case WM_CLOSE: {
        if (!w->opts.closable) break;

        SSC::JSON::Object json = SSC::JSON::Object::Entries {
          {"data", w->index}
        };

        auto app = App::sharedApplication();
        app->windowManager->destroyWindow(w->index);

        for (auto window : app->windowManager->windows) {
          if (window != nullptr) {
            window->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));
          }
        }

        w->close(0);
        break;
      }

      case WM_HOTKEY: {
        w->hotkey.onHotKeyBindingCallback((HotKeyBinding::ID) wParam);
        break;
      }

      case WM_HANDLE_DEEP_LINK: {
        auto url = SSC::String((const char*) lParam, wParam);
        SSC::JSON::Object json = SSC::JSON::Object::Entries {{
          "url", url
        }};

        if (app != nullptr && app->windowManager != nullptr) {
          for (auto window : app->windowManager->windows) {
            if (window != nullptr) {
              window->bridge->router.emit("applicationurl", json.str());
            }
          }
        }
        break;
      }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
  }

} // namespace SSC
