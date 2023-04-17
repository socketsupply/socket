#include <Shlwapi.h>
#include <objidl.h>
#include <wrl.h>
#include <shellapi.h>
#include "window.hh"

#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include "WebView2Experimental.h"
#include "WebView2ExperimentalEnvironmentOptions.h"

#pragma comment(lib, "Shlwapi.lib")

#ifndef CHECK_FAILURE
#define CHECK_FAILURE(...)
#endif

using namespace Microsoft::WRL;

namespace SSC {
  static inline void alert (const SSC::WString &ws) {
    MessageBoxA(nullptr, SSC::WStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
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

        GlobalUnlock(list);
        ReleaseStgMedium(&medium);

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

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    app.isReady = false;

    window = CreateWindow(
      TEXT("DesktopApp"), TEXT("Socket"),
      WS_OVERLAPPEDWINDOW,
      100000,
      100000,
      opts.width, opts.height,
      NULL, NULL,
      app.hInstance, NULL
    );

    HRESULT initResult = OleInitialize(NULL);

    this->drop = new DragDrop(this);

    this->bridge = new IPC::Bridge(app.core);
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
    ShowWindow(window, SW_SHOW);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) this);

    SSC::String preload = createPreload(opts);

    wchar_t modulefile[MAX_PATH];
    GetModuleFileNameW(NULL, modulefile, MAX_PATH);
    auto file = (fs::path { modulefile }).filename();
    auto filename = SSC::StringToWString(file.string());
    auto path = SSC::StringToWString(getEnv("APPDATA"));
    this->modulePath = fs::path(modulefile);

    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(L"--allow-file-access-from-files");

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> optionsExperimental;
    HRESULT oeResult = options.As(&optionsExperimental);
    if (oeResult != S_OK) {
      // UNREACHABLE - cannot continue
    }

    // TODO(@mribbons): Get Socket scheme working
    const WCHAR* customSchemes[2] = { L"ipc", L"socket" };
    const WCHAR* allowedSchemeOrigins[3] = { L"http://*", L"https://*", L"file://*" };

    auto ipcSchemeRegistration = Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(L"ipc");
    ipcSchemeRegistration->put_TreatAsSecure(TRUE);
    ipcSchemeRegistration->put_HasAuthorityComponent(TRUE);
    ipcSchemeRegistration->SetAllowedOrigins(3, allowedSchemeOrigins);

    auto socketSchemeRegistration = Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(L"socket");
    socketSchemeRegistration->put_TreatAsSecure(TRUE);
    socketSchemeRegistration->put_HasAuthorityComponent(TRUE);
    socketSchemeRegistration->SetAllowedOrigins(3, allowedSchemeOrigins);

    // If someone can figure out how to allocate this so we can do it in a loop that'd be great, but even Ms is doing it like this:
    // https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2environmentoptions4?view=webview2-1.0.1587.40
    ICoreWebView2CustomSchemeRegistration* registrations[2] = {
      ipcSchemeRegistration.Get(), 
      socketSchemeRegistration.Get()
    };

    optionsExperimental->SetCustomSchemeRegistrations(2, static_cast<ICoreWebView2CustomSchemeRegistration**>(registrations));

    auto init = [&]() -> HRESULT {
      return CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        (path + L"/" + filename).c_str(),
        options.Get(),
        Microsoft::WRL::Callback<IEnvHandler>(
          [&, preload](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
            env->CreateCoreWebView2Controller(
              window,
              Microsoft::WRL::Callback<IConHandler>(
                [&, preload](HRESULT result, ICoreWebView2Controller* c) -> HRESULT {
                  if (c != nullptr) {
                    controller = c;
                    controller->get_CoreWebView2(&webview);

                    RECT bounds;
                    GetClientRect(window, &bounds);
                    controller->put_Bounds(bounds);
                    controller->AddRef();
                  }

                  ICoreWebView2Settings* Settings;
                  webview->get_Settings(&Settings);
                  Settings->put_IsScriptEnabled(TRUE);
                  Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                  Settings->put_IsWebMessageEnabled(TRUE);
                  Settings->put_IsStatusBarEnabled(FALSE);

                  Settings->put_AreDefaultContextMenusEnabled(TRUE);
                  if (isDebugEnabled()) {
                    Settings->put_AreDevToolsEnabled(TRUE);
                  } else {
                    Settings->put_AreDevToolsEnabled(FALSE);
                  }

                  Settings->put_IsBuiltInErrorPageEnabled(FALSE);
                  Settings->put_IsZoomControlEnabled(FALSE);

                  auto settings3 = (ICoreWebView2Settings3*) Settings;
                  settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);

                  auto settings6 = (ICoreWebView2Settings6*) Settings;
                  settings6->put_IsPinchZoomEnabled(FALSE);
                  settings6->put_IsSwipeNavigationEnabled(FALSE);

                  EnumChildWindows(window, [](HWND hWnd, LPARAM window) -> BOOL {
                    int l = GetWindowTextLengthW(hWnd);

                    if (l > 0) {
                      wchar_t* buf = new wchar_t[l+1];
                      GetWindowTextW(hWnd, buf, l+1);

                      if (SSC::WStringToString(buf).find("Chrome") != SSC::String::npos) {
                        RevokeDragDrop(hWnd);
                        Window* w = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)window, GWLP_USERDATA));
                        w->drop->childWindow = hWnd;
                        RegisterDragDrop(hWnd, w->drop);
                      }
                    }
                    return TRUE;
                  }, (LPARAM)window);

                  app.isReady = true;

                  EventRegistrationToken tokenNavigation;

                  webview->add_NavigationStarting(
                    Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                      [&](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs *e) {
                        PWSTR uri;
                        e->get_Uri(&uri);
                        SSC::String url(SSC::WStringToString(uri));

                        if (url.find("socket:") != 0 && url.find("file://") != 0 && url.find("http://localhost") != 0) {
                          e->put_Cancel(true);
                        }

                        CoTaskMemFree(uri);
                        return S_OK;
                      }
                    ).Get(),
                    &tokenNavigation
                  );

                  EventRegistrationToken tokenSchemaFilter;
                  webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST);
                  webview->AddWebResourceRequestedFilter(L"socket:*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                  webview->AddWebResourceRequestedFilter(L"socket:*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST);
                  webview->add_WebResourceRequested(
                    Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                      [&](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) {
                        Window* w = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)window, GWLP_USERDATA));
                        ICoreWebView2_2* webview2 = nullptr;
                        ICoreWebView2Environment* env = nullptr;
                        ICoreWebView2WebResourceResponse* res = nullptr;
                        ICoreWebView2WebResourceRequest* req = nullptr;
                        String method_s;
                        String uri_s;
                        LPWSTR req_uri;
                        LPWSTR method;

                        webview->QueryInterface(IID_PPV_ARGS(&webview2));
                        webview2->get_Environment(&env);
                        args->get_Request(&req);
                        req->get_Uri(&req_uri);
                        uri_s = WStringToString(req_uri);
                        CoTaskMemFree(req_uri);

                        bool ipc_scheme = false;
                        bool socket_scheme = false;
                        bool handled = false;

                        if (uri_s.compare(0, 4, "ipc:") == 0) {
                          ipc_scheme = true;
                        } else if (uri_s.compare(0, 7, "socket:") == 0) {
                          socket_scheme = true;
                        } else {
                          return S_OK;
                        }

                        req->get_Method(&method);
                        method_s = WStringToString(method);                        
                        CoTaskMemFree(method);

                        // Handle CORS preflight request.
                        if (method_s.compare("OPTIONS") == 0) {
                          env->CreateWebResourceResponse(
                            nullptr,
                            204,
                            L"OK",
                            L"Connection: keep-alive\n"
                            L"Access-Control-Allow-Headers: *\n"
                            L"Access-Control-Allow-Origin: *\n"
                            L"Access-Control-Allow-Methods: GET, POST, PUT\n",
                            &res
                          );
                          args->put_Response(res);

                          return S_OK;
                        }

                        // WebView2 doesn't handle the trailing slash well. Remove it.
                        if (uri_s.ends_with("/")) uri_s = uri_s.substr(0, uri_s.size()-1);

                        ICoreWebView2Deferral* deferral;
                        HRESULT hr = args->GetDeferral(&deferral);

                        char* body_ptr = nullptr;
                        size_t body_length = 0;

                        if (ipc_scheme) {
                          if (method_s.compare("POST") == 0 || method_s.compare("PUT") == 0) {
                            IStream* body_data;
                            DWORD actual;
                            HRESULT r;
                            auto msg = IPC::Message{uri_s};
                            // TODO(trevnorris): Make sure index and seq are set.
                            if (w->bridge->router.hasMappedBuffer(msg.index, msg.seq)) {
                              IPC::MessageBuffer buf = w->bridge->router.getMappedBuffer(msg.index, msg.seq);
                              ICoreWebView2ExperimentalSharedBuffer* shared_buf = buf.shared_buf;
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
                          } else if (method_s.compare("GET") != 0) {
                            // UNREACHABLE
                          }

                          handled = w->bridge->route(uri_s, body_ptr, body_length, [&, args, deferral, env, body_ptr](auto result) {
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
                                StringToWString(headers).c_str(),
                                &res
                              );
                              args->put_Response(res);
                              deferral->Complete();
                              delete[] body;
                            });
                          });
                        }

                        if (socket_scheme) {
                          if (method_s.compare("GET") == 0) {
                            if (uri_s.starts_with("socket:///")) {
                              uri_s = uri_s.substr(10);
                            } else if (uri_s.starts_with("socket://")) {
                              uri_s = uri_s.substr(9);
                            } else if (uri_s.starts_with("socket:")) {
                              uri_s = uri_s.substr(7);
                            }

                            if (uri_s.ends_with("/")) {
                              uri_s = uri_s.substr(0, uri_s.size() - 1);
                            }


                            auto ext = uri_s.ends_with(".js") ? "" : ".js";

                            // look for socket lib in initial app folder
                            auto rootPath = this->modulePath.parent_path();
                            auto path = rootPath / "socket" / (uri_s + ext);

                            if (!fs::exists(path)) {
                              auto path = fs::path(fs::current_path()) / (uri_s + ext);
                            }

                            if (!fs::exists(path)) {
                              path = fs::path(fs::current_path()) / "socket" / (uri_s + ext);
                            }

                            if (fs::exists(path)) {
                              String headers;
                              char* body;

                              auto moduleUri = "file://" + replace(path.string(), "\\\\", "/");
                              auto moduleSource = trim(tmpl(
                                moduleTemplate,
                                Map { {"url", String(moduleUri)} }
                              ));

                              size_t length = moduleSource.size();
                              body = new char[length];
                              memcpy(body, moduleSource.c_str(), length);
                              headers = "Content-Type: text/javascript\n";
                              headers += "Connection: keep-alive\n";
                              headers += "Access-Control-Allow-Headers: *\n";
                              headers += "Access-Control-Allow-Origin: *\n";
                              headers += "Content-Length: ";
                              headers += std::to_string(length);
                              headers += "\n";

                              app.dispatch([&, body, length, headers, args, deferral, env] {
                                ICoreWebView2WebResourceResponse* res = nullptr;
                                IStream* bytes = SHCreateMemStream((const BYTE*)body, length);
                                env->CreateWebResourceResponse(
                                  bytes,
                                  200,
                                  L"OK",
                                  StringToWString(headers).c_str(),
                                  &res
                                );
                                args->put_Response(res);
                                deferral->Complete();
                                delete[] body;
                              });
                              handled = true;
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
                        // TODO(trevnorris): Called when window.open() is called in JS, but the new
                        // window won't have all the setup and request interception. This setup should
                        // be moved to another location where it can be run for any new window. Right
                        // now ipc won't work for any new window.
                        e->put_Handled(true);
                        return S_OK;
                      }
                    ).Get(),
                    &tokenNewWindow
                  );

                  webview->AddScriptToExecuteOnDocumentCreated(
                    SSC::StringToWString(preload).c_str(),
                    Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                      [&](HRESULT error, PCWSTR id) -> HRESULT {
                        return S_OK;
                      }
                    ).Get()
                  );

                  EventRegistrationToken tokenMessage;

                  webview->add_WebMessageReceived(
                    Microsoft::WRL::Callback<IRecHandler>([&](ICoreWebView2* webview, IArgs* args) -> HRESULT {
                      LPWSTR messageRaw;
                      args->TryGetWebMessageAsString(&messageRaw);                      
                      SSC::WString message_w(messageRaw);
                      CoTaskMemFree(messageRaw);
                      if (onMessage != nullptr) {
                        SSC::String message = SSC::WStringToString(message_w);
                        auto msg = IPC::Message{message};
                        Window* w = reinterpret_cast<Window*>(GetWindowLongPtr((HWND)window, GWLP_USERDATA));
                        ICoreWebView2_2* webview2 = nullptr;
                        ICoreWebView2Environment* env = nullptr;
                        ICoreWebView2Experimental18* webView18 = nullptr;
                        ICoreWebView2ExperimentalEnvironment10* environment = nullptr;

                        webview->QueryInterface(IID_PPV_ARGS(&webview2));
                        webview2->get_Environment(&env);
                        env->QueryInterface(IID_PPV_ARGS(&environment));

                        webview->QueryInterface(IID_PPV_ARGS(&webView18));

                        // this should only come from `postMessage()`
                        if (msg.name == "buffer.create") {
                          auto seq = msg.seq;
                          auto size = std::stoull(msg.get("size", "0"));
                          auto index = msg.index;
                          ICoreWebView2ExperimentalSharedBuffer* sharedBuffer = nullptr;
                          // TODO(trevnorris): What to do if creation fails, or size == 0?
                          HRESULT cshr = environment->CreateSharedBuffer(size, &sharedBuffer);
                          String additionalData = "{\"seq\":\"";
                          additionalData += seq;
                          additionalData += "\",\"index\":";
                          additionalData += std::to_string(index);
                          additionalData += "}";
                          cshr = webView18->PostSharedBufferToScript(
                            sharedBuffer,
                            COREWEBVIEW2_SHARED_BUFFER_ACCESS_READ_WRITE,
                            StringToWString(additionalData).c_str()
                          );
                          IPC::MessageBuffer msg_buf(sharedBuffer, size);
                          // TODO(trevnorris): This will leak memory if the buffer is created and
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
                      COREWEBVIEW2_PERMISSION_KIND kind;
                      args->get_PermissionKind(&kind);

                      if (kind == COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ) {
                        args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
                      }

                      return S_OK;
                    }).Get(),
                    &tokenPermissionRequested
                  );

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
      // TODO(trevnorris):
      // 1. power-shell-out to download and run webview installer
      // 2. restart app
    }
  }

  ScreenSize Window::getScreenSize () {
    return ScreenSize {
      .height = GetSystemMetrics(SM_CYFULLSCREEN),
      .width = GetSystemMetrics(SM_CXFULLSCREEN)
    };
  }

  void Window::about () {
    auto text = SSC::String(
      app.appData["build_name"] + " " +
      "v" + app.appData["meta_version"] + "\n" +
      "Built with ssc v" + SSC::VERSION_FULL_STRING + "\n" +
      app.appData["meta_copyright"]
    );

    MSGBOXPARAMS mbp;
    mbp.cbSize = sizeof(MSGBOXPARAMS);
    mbp.hwndOwner = window;
    mbp.hInstance = app.hInstance;
    mbp.lpszText = text.c_str();
    mbp.lpszCaption = app.appData["build_name"].c_str();
    mbp.dwStyle = MB_USERICON;
    mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    mbp.lpfnMsgBoxCallback = NULL;
    mbp.dwContextHelpId = 0;

    MessageBoxIndirect(&mbp);
  }

  void Window::kill () {
    if (this->controller != nullptr) this->controller->Close();
    if (this->window != nullptr) DestroyWindow(this->window);
  }

  void Window::showInspector () {
    // TODO: show inspector.
  }

  void Window::exit (int code) {
    if (this->onExit != nullptr) 
    {
      std::cerr << "WARNING: Window#" << index << " exiting with code " << code << std::endl;
      this->onExit(code);
    }
    else {
      std::cerr << "WARNING: Window#" << index << " window->onExit is null in Window::exit()" << std::endl;
    }
  }

  void Window::close (int code) {
    if (opts.canExit) {
      this->exit(0);
      DestroyWindow(window);
    } else {
      this->hide();
    }
  }

  void Window::show () {
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
    this->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
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
        SSC::StringToWString(s).c_str(),
        nullptr
      );
    });
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

      webview->Navigate(SSC::StringToWString(value).c_str());
    });
  }

  SSC::String Window::getTitle () {
    int len = GetWindowTextLength(window) + 1;
    LPTSTR title = new TCHAR[len];
    GetWindowText(window, title, len);
    String title_s = WStringToString(title);
    delete[] title;
    return title_s;
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
      height = rect.bottom - rect.top;
      width = rect.right - rect.left;
    }

    return { static_cast<int>(height), static_cast<int>(width) };
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
      m_maxsz.x = width;
      m_maxsz.y = height;
    } else if (hints == WINDOW_HINT_MIN) {
      m_minsz.x = width;
      m_minsz.y = height;
    } else {
      RECT r;
      r.left = r.top = 0;
      r.right = width;
      r.bottom = height;

      AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);

      SetWindowPos(
        window,
        NULL,
        r.left, r.top, r.right - r.left, r.bottom - r.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
      );

      resize(window);
    }

    this->width = width;
    this->height = height;
  }

  void Window::setSystemMenu (const SSC::String& seq, const SSC::String& value) {
    SSC::String menu = value;

    HMENU hMenubar = GetMenu(window);

    // deserialize the menu
    menu = replace(menu, "%%", "\n");

    // split on ;
    auto menus = split(menu, ';');
    int itemId = 0;

    for (auto m : menus) {
      auto menu = split(m, '\n');
      auto line = trim(menu[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];

      HMENU hMenu = CreateMenu();

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.size() == 0) continue;

        if (line.empty()) {
          continue;
        }

        if (line.find("---") != -1) {
          AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
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
          }

          if (isShift) {
            accl = SSC::String("Shift+" + accl);
          }
        }

        auto display = SSC::String(title + "\t" + accl);
        AppendMenuA(hMenu, MF_STRING, itemId, display.c_str());
        menuMap[itemId] = SSC::String(title + "\t" + menuTitle);
        itemId++;
      }

      AppendMenuA(hMenubar, MF_POPUP, (UINT_PTR) hMenu, menuTitle.c_str());
    }

    MENUINFO Info;
    Info.cbSize = sizeof(Info);
    Info.fMask = MIM_BACKGROUND | MFT_OWNERDRAW;
    Info.hbrBack = CreateSolidBrush(RGB(0, 0, 0));
    SetMenuInfo(hMenubar, &Info);

    RECT rc;
    rc.top = 0;
    rc.left = 0;
    rc.bottom = 0;
    rc.right = 0;
    InvalidateRect(this->window, &rc, true);
    DrawMenuBar(this->window);
    RedrawWindow(this->window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

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

  void Window::setContextMenu (const SSC::String& seq, const SSC::String& value) {
    HMENU hPopupMenu = CreatePopupMenu();

    auto menuItems = split(value, '_');
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
    this->eval(getResolveMenuSelectionJavaScript(seq, lookup.at(selection), "contextMenu"));
  }

  int Window::openExternal (const SSC::String& url) {
    ShellExecute(nullptr, "Open", url .c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    // TODO how to detect success here. do we care?
    return 0;
  }

  void Window::setBackgroundColor(int r, int g, int b, float a) {
    SetBkColor(GetDC(window), RGB(r, g, b));
    app.wcex.hbrBackground = CreateSolidBrush(RGB(r, g, b));
  }

  void Window::openDialog (
      const SSC::String& seq,
      bool isSave,
      bool allowDirs,
      bool allowFiles,
      bool allowMultiple,
      const SSC::String& defaultPath,
      const SSC::String& title,
      const SSC::String& defaultName)
  {
    std::vector<SSC::String> result_paths;
    SSC::String result_string = "";
    IShellItemArray *results;
    IShellItem *single_result;
    DWORD dialog_options;
    DWORD results_count;
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
      std::cerr << "ERR: CoInitializeEx() failed in 'openDialog()'" << std::endl;
      return;
    }

    // create IFileDialog instance (IFileOpenDialog or IFileSaveDialog)
    if (isSave) {
      result = CoCreateInstance(
        CLSID_FileSaveDialog,
        NULL,
        CLSCTX_ALL,
        IID_PPV_ARGS(&dialog.save)
      );

      if (FAILED(result)) {
        std::cerr << "ERR: CoCreateInstance() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    } else {
      result = CoCreateInstance(
        CLSID_FileOpenDialog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog.open)
      );

      if (FAILED(result)) {
        std::cerr << "ERR: CoCreateInstance() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (isSave) {
      result = dialog.save->GetOptions(&dialog_options);
    } else {
      result = dialog.open->GetOptions(&dialog_options);
    }

    if (FAILED(result)) {
      std::cerr << "ERR: IFileDialog::GetOptions() failed in 'openDialog()'" << std::endl;
      CoUninitialize();
      return;
    }

    if (allowDirs == true && allowFiles == false) {
      if (isSave) {
        result = dialog.save->SetOptions(dialog_options | FOS_PICKFOLDERS);
      } else {
        result = dialog.open->SetOptions(dialog_options | FOS_PICKFOLDERS);
      }

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::SetOptions(FOS_PICKFOLDERS) failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if ((!isSave || (!isSave && allowDirs)) && allowMultiple) {
      result = dialog.open->SetOptions(dialog_options | FOS_ALLOWMULTISELECT);

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::SetOptions(FOS_ALLOWMULTISELECT) failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (!defaultPath.empty()) {
      IShellItem *defaultFolder;

      auto normalizedDefaultPath = defaultPath;
      std::replace(normalizedDefaultPath.begin(), normalizedDefaultPath.end(), '/', '\\');
      result = SHCreateItemFromParsingName(
        SSC::WString(normalizedDefaultPath.begin(), normalizedDefaultPath.end()).c_str(),
        NULL,
        IID_PPV_ARGS(&defaultFolder)
      );

      if (FAILED(result)) {
        std::cerr << "ERR: SHCreateItemFromParsingName() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }

      if (isSave) {
        result = dialog.save->SetDefaultFolder(defaultFolder);
      } else {
        result = dialog.open->SetDefaultFolder(defaultFolder);
      }

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::SetDefaultFolder() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (!title.empty()) {
      if (isSave) {
        result = dialog.save->SetTitle(
          SSC::WString(title.begin(), title.end()).c_str()
        );
      } else {
        result = dialog.open->SetTitle(
          SSC::WString(title.begin(), title.end()).c_str()
        );
      }

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::SetTitle() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (!defaultName.empty()) {
      if (isSave) {
        result = dialog.save->SetFileName(
          SSC::WString(defaultName.begin(), defaultName.end()).c_str()
        );
      } else {
        result = dialog.open->SetFileName(
          SSC::WString(defaultName.begin(), defaultName.end()).c_str()
        );
      }

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::SetFileName() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (isSave) {
      result = dialog.save->Show(NULL);
    } else {
      result = dialog.open->Show(NULL);
    }

    if (FAILED(result)) {
      std::cerr << "ERR: IFileDialog::Show() failed in 'openDialog()'" << std::endl;
      CoUninitialize();
      return;
    }

    if (isSave) {
      result = dialog.save->GetResult(&single_result);

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::GetResult() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    } else {
      result = dialog.open->GetResults(&results);

      if (FAILED(result)) {
        std::cerr << "ERR: IFileDialog::GetResults() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }
    }

    if (FAILED(result)) {
      std::cerr << "ERR: IFileDialog::Show() failed in 'openDialog()'" << std::endl;
      CoUninitialize();
      return;
    }

    if (isSave) {
      LPWSTR buf;

      result = single_result->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &buf);

      if (FAILED(result)) {
        std::cerr << "ERR: IShellItem::GetDisplayName() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }

      result_paths.push_back(SSC::WStringToString(SSC::WString(buf)));
      single_result->Release();

      CoTaskMemFree(buf);
    } else {
      results->GetCount(&results_count);

      if (FAILED(result)) {
        std::cerr << "ERR: IShellItemArray::GetCount() failed in 'openDialog()'" << std::endl;
        CoUninitialize();
        return;
      }

      for (DWORD i = 0; i < results_count; i++) {
        IShellItem *path;
        LPWSTR buf;

        result = results->GetItemAt(i, &path);

        if (FAILED(result)) {
          std::cerr << "ERR: IShellItemArray::GetItemAt() failed in 'openDialog()'" << std::endl;
          CoUninitialize();
          return;
        }

        result = path->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &buf);

        if (FAILED(result)) {
          std::cerr << "ERR: IShellItem::GetDisplayName() failed in 'openDialog()'" << std::endl;
          CoUninitialize();
          return;
        }

        result_paths.push_back(SSC::WStringToString(SSC::WString(buf)));
        path->Release();
        CoTaskMemFree(buf);
      }
    }

      for (size_t i = 0, i_end = result_paths.size(); i < i_end; ++i) {
        result_string += (i ? "\\n" : "");
        std::replace(result_paths[i].begin(), result_paths[i].end(), '\\', '/');
        result_string += result_paths[i];
      }

    auto wrapped_result_string =  String("\"" + result_string + "\"");
    this->resolvePromise(seq, "0", encodeURIComponent(wrapped_result_string));

    if (isSave) {
      dialog.save->Release();
    } else {
      dialog.open->Release();
    }

    if (!isSave) {
      results->Release();
    }

    CoUninitialize();
  }

  // message is defined in WinUser.h
  // https://raw.githubusercontent.com/tpn/winsdk-10/master/Include/10.0.10240.0/um/WinUser.h
  LRESULT CALLBACK Window::WndProc(
      HWND hWnd,
      UINT message,
      WPARAM wParam,
      LPARAM lParam) {

    Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
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

      case WM_COMMAND: {
        if (w == nullptr) {
          break;
        }

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

          w->eval(getResolveMenuSelectionJavaScript("0", title, parent));
        }

        break;
      }

      case WM_SETTINGCHANGE: {
        // TODO(trevnorris): Dark mode
        break;
      }

      case WM_CREATE: {
        // TODO(trevnorris): Dark mode
        SetWindowTheme(hWnd, L"Explorer", NULL);
        HMENU hMenubar = CreateMenu();
        SetMenu(hWnd, hMenubar);
        break;
      }

      case WM_CLOSE: {
        w->close(0);
        break;
      }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
  }

} // namespace SSC
