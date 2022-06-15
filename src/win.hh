#include "common.hh"
#include "win64/WebView2.h"
#include "win64/WebView2EnvironmentOptions.h"

#include <signal.h>
#include <future>
#include <chrono>
#include <shobjidl.h>
#include <shlobj_core.h>

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"WebView2LoaderStatic.lib")

// for dark mode...
#include <uxtheme.h>
#include <dwmapi.h>
#include <wingdi.h>

#pragma comment(lib,"UxTheme.lib")
#pragma comment(lib,"Dwmapi.lib")
#pragma comment(lib,"Gdi32.lib")

inline void alert (const std::wstring &ws) {
  MessageBoxA(nullptr, SSC::WStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

inline void alert (const std::string &s) {
  MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

inline void alert (const char* s) {
  MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

namespace SSC {
  using IEnvHandler = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
  using IConHandler = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
  using INavHandler = ICoreWebView2NavigationCompletedEventHandler;
  using IRecHandler = ICoreWebView2WebMessageReceivedEventHandler;
  using IArgs = ICoreWebView2WebMessageReceivedEventArgs;

  class App: public IApp {
    public:
      MSG msg;
      WNDCLASSEX wcex;
      _In_ HINSTANCE hInstance;
      DWORD mainThread = GetCurrentThreadId();

      static std::atomic<bool> isReady;

      App(void* h);
      int run();
      void exit(int code);
      void kill();
      void restart();
      void dispatch(std::function<void()>);
      std::string getCwd(const std::string&);
  };

  std::atomic<bool> App::isReady {false};

	// constexpr COLORREF darkBkColor = 0x383838;
	// constexpr COLORREF darkTextColor = 0xFFFFFF;
	// static HBRUSH hbrBkgnd = nullptr;

  enum WINDOWCOMPOSITIONATTRIB {
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
  };

  struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
  };

  using RefreshImmersiveColorPolicyState = VOID(WINAPI*)();
  using ShouldSystemUseDarkMode = BOOL(WINAPI*)();
  using AllowDarkModeForApp = BOOL(WINAPI*)(BOOL allow);
  using SetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);

  static RefreshImmersiveColorPolicyState refreshImmersiveColorPolicyState = nullptr;
  static ShouldSystemUseDarkMode shouldSystemUseDarkMode = nullptr;
  static AllowDarkModeForApp allowDarkModeForApp = nullptr;
  static SetWindowCompositionAttribute setWindowCompositionAttribute = nullptr;

  static auto bgBrush = CreateSolidBrush(RGB(0, 0, 0));
  FILE* console;

  class DragDrop;

  class Window : public IWindow {
    DWORD mainThread = GetCurrentThreadId();

    public:
      HWND window;
      Window(App&, WindowOptions);
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      ICoreWebView2 *webview = nullptr;
      ICoreWebView2Controller *controller = nullptr;
      DragDrop* drop;

      App app;
      WindowOptions opts;
      std::map<int, std::string> menuMap;

      void resize (HWND window);
      POINT m_minsz = POINT {0, 0};
      POINT m_maxsz = POINT {0, 0};
      HMENU systemMenu;

      void about();
      void eval(const std::string&);
      void show(const std::string&);
      void hide(const std::string&);
      void exit(int code);
      void kill();
      void close(int code);
      void navigate(const std::string&, const std::string&);
      void setSize(const std::string&, int, int, int);
      void setTitle(const std::string&, const std::string&);
      void setContextMenu(const std::string&, const std::string&);
      void closeContextMenu(const std::string&);
      void closeContextMenu();
      void showInspector();
      void setSystemMenu(const std::string&, const std::string&);
      void setSystemMenuItemEnabled(bool enabled, int barPos, int menuPos);
      void openDialog(
        const std::string&,
        bool,
        bool,
        bool,
        bool,
        const std::string&,
        const std::string&,
        const std::string&
      );
      int openExternal(const std::string&);
      ScreenSize getScreenSize();
      void setBackgroundColor(int r, int g, int b, float a);
  };

  class CDataObject : public IDataObject {
    public:
      HRESULT __stdcall QueryInterface (REFIID iid, void ** ppvObject);
      ULONG   __stdcall AddRef (void);
      ULONG   __stdcall Release (void);

      HRESULT __stdcall GetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
      HRESULT __stdcall GetDataHere			(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium);
      HRESULT __stdcall QueryGetData			(FORMATETC *pFormatEtc);
      HRESULT __stdcall GetCanonicalFormatEtc (FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
      HRESULT __stdcall SetData				(FORMATETC *pFormatEtc,  STGMEDIUM *pMedium,  BOOL fRelease);
      HRESULT __stdcall EnumFormatEtc			(DWORD      dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
      HRESULT __stdcall DAdvise				(FORMATETC *pFormatEtc,  DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
      HRESULT __stdcall DUnadvise				(DWORD      dwConnection);
      HRESULT __stdcall EnumDAdvise			(IEnumSTATDATA **ppEnumAdvise);

      CDataObject(FORMATETC *fmt, STGMEDIUM *stgmed, int count);
      ~CDataObject();

    private:
      int LookupFormatEtc(FORMATETC *pFormatEtc);
      LONG m_lRefCount;
      FORMATETC *m_pFormatEtc;
      STGMEDIUM *m_pStgMedium;
      LONG m_nNumFormats;
  };

  CDataObject::CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count) {
    m_lRefCount  = 1;
    m_nNumFormats = count;

    m_pFormatEtc  = new FORMATETC[count];
    m_pStgMedium  = new STGMEDIUM[count];

    for(int i = 0; i < count; i++) {
      m_pFormatEtc[i] = fmtetc[i];
      m_pStgMedium[i] = stgmed[i];
    }
  }

  CDataObject::~CDataObject() {
    if(m_pFormatEtc) delete[] m_pFormatEtc;
    if(m_pStgMedium) delete[] m_pStgMedium;
  }

  ULONG __stdcall CDataObject::AddRef(void) {
    return InterlockedIncrement(&m_lRefCount);
  }

  ULONG __stdcall CDataObject::Release(void) {
    LONG count = InterlockedDecrement(&m_lRefCount);

    if (count == 0) {
      delete this;
      return 0;
    } else {
      return count;
    }
  }

  HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject){
    if (iid == IID_IDataObject || iid == IID_IUnknown) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    } else {
      *ppvObject = 0;
      return E_NOINTERFACE;
    }
  }

  HGLOBAL DupMem(HGLOBAL hMem) {
    DWORD len = GlobalSize(hMem);
    PVOID source = GlobalLock(hMem);
    PVOID dest = GlobalAlloc(GMEM_FIXED, len);

    memcpy(dest, source, len);
    GlobalUnlock(hMem);
    return dest;
  }

  int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc) {
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
    std::vector<std::string> draggablePayload;
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

    HRESULT DragEnter (
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
        draggablePayload = SSC::split(std::string(list), ';');

        GlobalUnlock(list);
        ReleaseStgMedium(&medium);

        std::string json = (
          "{"
          "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
          "  \"inbound\": true,"
          "  \"x\":" + std::to_string(point.x) + ","
          "  \"y\":" + std::to_string(point.y) + ""
          "}"
        );

        auto payload = SSC::emitToRenderProcess("drag", json);
        this->window->eval(payload);
      }

      return S_OK;
    };

    ULONG AddRef (void) {
      refCount++;

      return refCount;
    }

    ULONG Release (void) {
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

      std::string json = (
        "{"
        "  \"count\":" + std::to_string(this->draggablePayload.size()) + ","
        "  \"inbound\": false,"
        "  \"x\":" + std::to_string(point.x) + ","
        "  \"y\":" + std::to_string(point.y) + ""
        "}"
      );

      auto payload = SSC::emitToRenderProcess("drag", json);
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

      std::vector<std::string> files = this->draggablePayload;

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
      for (std::vector<std::string>::size_type i = 0; i < files.size(); ++i) {
        std::string &file = files[i];

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

      std::stringstream filesStringArray;

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
            << SSC::replace(std::string(buf), "\\\\", "\\\\")
            << '"';

          if (i < count - 1) {
            filesStringArray << ",";
          }

          delete[] buf;
        }

        this->window->eval(
          "(() => {"
          "  const value = {"
          "    \"files\": [" + filesStringArray.str() + "],"
          "    \"x\": " + std::to_string(point.x) + " / window.devicePixelRatio,"
          "    \"y\": " + std::to_string(point.y) + " / window.devicePixelRatio"
          "  };"
          "  window._ipc.emit('dropin', JSON.stringify(value));"
          "})()"
        );

      } else {
        format.cfFormat = CF_TEXT;
        if (
          SUCCEEDED(dataObject->QueryGetData(&format)) &&
          SUCCEEDED(dataObject->GetData(&format, &medium))
        ) {
          *dragEffect = DROPEFFECT_MOVE;
          for (auto &src : this->draggablePayload) {
            std::string json = (
              "{"
              "  \"src\": \"" + src + "\","
              "  \"x\":" + std::to_string(point.x) + ","
              "  \"y\":" + std::to_string(point.y) + ""
              "}"
            );

            this->window->eval(SSC::emitToRenderProcess("drop", json));
          }

          std::string json = (
            "{"
            "  \"x\":" + std::to_string(point.x) + ","
            "  \"y\":" + std::to_string(point.y) + ""
            "}"
          );

          this->window->eval(SSC::emitToRenderProcess("dragend", json));
        }
      }

      this->draggablePayload.clear();

      GlobalUnlock(medium.hGlobal);
      ReleaseStgMedium(&medium);

      return S_OK;
    };
  };

  App::App(void* h): hInstance((_In_ HINSTANCE) h) {
    #if DEBUG == 1
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    #endif

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttribute>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"),
      "SetWindowCompositionAttribute")
    );

    if (hUxtheme) {
      refreshImmersiveColorPolicyState =
        (RefreshImmersiveColorPolicyState) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));

      shouldSystemUseDarkMode =
        (ShouldSystemUseDarkMode) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138));

      allowDarkModeForApp =
        (AllowDarkModeForApp) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    }

    allowDarkModeForApp(shouldSystemUseDarkMode());
    refreshImmersiveColorPolicyState();

    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    auto iconPath = fs::path { getCwd("") / fs::path { "index.ico" } };

    HICON icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CXSMICON),
      LR_LOADFROMFILE
    );

    auto *szWindowClass = L"DesktopApp";
    auto *szTitle = L"Socket SDK";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = bgBrush;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    wcex.lpfnWndProc = Window::WndProc;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  };

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    app.isReady = false;

    window = CreateWindow(
      TEXT("DesktopApp"), TEXT("Socket SDK"),
      WS_OVERLAPPEDWINDOW,
      100000,
      100000,
      1024, 780,
      NULL, NULL,
      app.hInstance, NULL
    );

    /* BOOL mode = FALSE;

    if (shouldSystemUseDarkMode()) {
      allowDarkModeForApp(true);
      refreshImmersiveColorPolicyState();
      mode = TRUE;
      // SetWindowTheme(window, L"DarkMode_Explorer", NULL);
    }

		WINDOWCOMPOSITIONATTRIBDATA data = {
      WCA_USEDARKMODECOLORS,
      &mode,
      sizeof(mode)
    };

		setWindowCompositionAttribute(window, &data);
    SetPropW(window, L"UseImmersiveDarkModeColors", (HANDLE)(LONG_PTR) mode);
    // SetPropW(window, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(mode)));

    // SetWindowTheme(window, L"Explorer", nullptr);
    // DwmSetWindowAttribute(window, 19, &mode, sizeof(long));
    */

    HRESULT initResult = OleInitialize(NULL);

    this->drop = new DragDrop(this);

    //
    // In theory these allow you to do drop files in elevated mode
    //
    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);

    UpdateWindow(window);
    ShowWindow(window, SW_SHOW);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) this);

    std::string preload(
      "window.external = {\n"
      "  invoke: arg => window.chrome.webview.postMessage(arg)\n"
      "};\n"
      "" + createPreload(opts) + "\n"
    );

    wchar_t modulefile[MAX_PATH];
    GetModuleFileNameW(NULL, modulefile, MAX_PATH);
    auto file = (fs::path { modulefile }).filename();
    auto filename = StringToWString(file.string());
    auto path = StringToWString(getEnv("APPDATA"));
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    options->put_AdditionalBrowserArguments(L"--allow-file-access-from-files");

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
                  hide("");

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
                  #if DEBUG == 1
                    Settings->put_AreDevToolsEnabled(TRUE);
                  #else 
                    Settings->put_AreDevToolsEnabled(FALSE);
                  #endif

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

                      if (WStringToString(buf).find("Chrome") != std::string::npos) {
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
                    Callback<ICoreWebView2NavigationStartingEventHandler>(
                      [&](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *e) {
                        PWSTR uri;
                        e->get_Uri(&uri);
                        std::string url(WStringToString(uri));

                        if (url.find("file://") != 0 && url.find("http://localhost") != 0) {
                          e->put_Cancel(true);
                        }

                        CoTaskMemFree(uri);
                        return S_OK;
                      }
                    ).Get(),
                    &tokenNavigation
                  );

                  EventRegistrationToken tokenNewWindow;

                  webview->add_NewWindowRequested(
                    Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                      [&](ICoreWebView2* wv, ICoreWebView2NewWindowRequestedEventArgs* e) {
                        // PWSTR uri;
                        // e->get_Uri(&uri);
                        // std::string url(WStringToString(uri));
                        e->put_Handled(true);
                        return S_OK;
                      }
                    ).Get(),
                    &tokenNewWindow
                  );

                  webview->AddScriptToExecuteOnDocumentCreated(
                    StringToWString(preload).c_str(),
                    Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                      [&](HRESULT error, PCWSTR id) -> HRESULT {
                        return S_OK;
                      }
                    ).Get()
                  );

                  EventRegistrationToken tokenMessage;

                  webview->add_WebMessageReceived(
                    Callback<IRecHandler>([&](ICoreWebView2* webview, IArgs* args) -> HRESULT {
                      LPWSTR messageRaw;
                      args->TryGetWebMessageAsString(&messageRaw);

                      if (onMessage != nullptr) {
                        std::wstring message(messageRaw);
                        onMessage(WStringToString(message));
                      }

                      CoTaskMemFree(messageRaw);
                      return S_OK;
                    }).Get(),
                    &tokenMessage
                  );

                  EventRegistrationToken tokenPermissionRequested;
                  webview->add_PermissionRequested(
                    Callback<ICoreWebView2PermissionRequestedEventHandler>([&](
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
      httplib::Client cli("https://go.microsoft.com");
      auto res = cli.Get("/fwlink/p/?LinkId=2124703");

      if (res->status != 200) {
        alert("Windows needs to be updated before it can run this software");
        return;
      }

      writeFile("webview2.exe", res->body);
      auto r = exec("webview2.exe");
      if (r.exitCode != 0) {
        // TODO: handle this
      }
    }
  }

  int App::run () {
    MSG msg;
    BOOL res = GetMessage(&msg, nullptr, 0, 0);

    if (msg.hwnd) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (msg.message == WM_APP) {
      auto f = (std::function<void()> *)(msg.lParam);
      (*f)();
      delete f;
    }

    if (msg.message == WM_QUIT) {
      if (shouldExit) return 1;
    }

    return 0;
  }

  void App::kill () {
    shouldExit = true;
    PostQuitMessage(0);
  }

  void App::restart () {
    char szFileName[MAX_PATH] = "";
    GetModuleFileName(NULL, szFileName, MAX_PATH);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    CreateProcess(NULL, szFileName, NULL, NULL, NULL, NULL, NULL, NULL, &si, &pi);

    std::exit(0);

    // WCHAR wsCommandLine[RESTART_MAX_CMD_LINE];
    // HRESULT hr = S_OK;             // not if registering for recovery and restart fails.

    // hr = RegisterApplicationRestart(wsCommandLine, RESTART_NO_PATCH | RESTART_NO_REBOOT);
    // if (FAILED(hr)) {
    //   alert("Failed to restart app");
    // }
  }

  void App::dispatch (std::function<void()> cb) {
    auto future = std::async(std::launch::async, [&] {
      while (!this->isReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      PostThreadMessage(
        mainThread,
        WM_APP,
        0,
        (LPARAM) new std::function<void()>(cb)
      );
    });

    future.get();
  }

  std::string App::getCwd (const std::string& _) {
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    auto path = fs::path { filename }.remove_filename();
    return path.string();
  }

  ScreenSize Window::getScreenSize () {
    return ScreenSize {
      .height = GetSystemMetrics(SM_CYFULLSCREEN),
      .width = GetSystemMetrics(SM_CXFULLSCREEN)
    };
  }

  void Window::about () {
    auto text = std::string(
      appData["title"] + " " +
      appData["version"] + "\n" +
      "Built with ssc v" + SSC::full_version + "\n" +
      appData["copyright"]
    );

    MSGBOXPARAMS mbp;
    mbp.cbSize = sizeof(MSGBOXPARAMS);
    mbp.hwndOwner = window;
    mbp.hInstance = app.hInstance;
    mbp.lpszText = text.c_str();
    mbp.lpszCaption = appData["title"].c_str();
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
    if (this->onExit != nullptr) this->onExit(code);
  }

  void Window::close (int code) {
    if (opts.canExit) {
      this->exit(0);
      DestroyWindow(window);
    } else {
      this->hide("");
    }
  }

  void Window::show (const std::string& seq) {
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

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::hide (const std::string& seq) {
    ShowWindow(window, SW_HIDE);
    UpdateWindow(window);
    this->eval(emitToRenderProcess("windowHide", "{}"));

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::resize (HWND window) {
    if (controller == nullptr) {
      return;
    }

    RECT bounds;
    GetClientRect(window, &bounds);
    controller->put_Bounds(bounds);
  }

  void Window::eval (const std::string& s) {
    app.dispatch([&, this, s] {
      this->webview->ExecuteScript(
        StringToWString(s).c_str(),
        nullptr
      );
    });
  }

  void Window::navigate (const std::string& seq, const std::string& value) {
    auto index = std::to_string(this->opts.index);

    app.dispatch([&, this, seq, value, index] {
      EventRegistrationToken token;
      this->webview->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [&, this, seq, index, token](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            std::string state = "1";

            BOOL success;
            args->get_IsSuccess(&success);

            if (success) {
              state = "0";
            }

            this->onMessage(resolveToMainProcess(seq, state, index));
            webview->remove_NavigationCompleted(token);

            return S_OK;
          })
        .Get(),
        &token
      );

      webview->Navigate(StringToWString(value).c_str());
    });
  }

  void Window::setTitle (const std::string& seq, const std::string& title) {
    SetWindowText(window, title.c_str());

    if (onMessage != nullptr) {
      std::string state = "0"; // can this call actually fail?
      auto index = std::to_string(this->opts.index);

      this->onMessage(resolveToMainProcess(seq, state, index));
    }
  }

  void Window::setSize (const std::string& seq, int width, int height, int hints) {
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

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::setSystemMenu (const std::string& seq, const std::string& value) {
    std::string menu = value;

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
        std::string key = "";

        auto accelerators = split(parts[1], '+');
        auto accl = std::string("");

        key = trim(parts[1]) == "_" ? "" : trim(accelerators[0]);

        if (key.size() > 0) {
          bool isShift = std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;
          accl = key;

          if (accelerators.size() > 1) {
            accl = std::string(trim(accelerators[1]) + "+" + key);
            accl = replace(accl, "CommandOrControl", "Ctrl");
            accl = replace(accl, "Command", "Ctrl");
            accl = replace(accl, "Control", "Ctrl");
          }

          if (isShift) {
            accl = std::string("Shift+" + accl);
          }
        }

        auto display = std::string(title + "\t" + accl);
        AppendMenuA(hMenu, MF_STRING, itemId, display.c_str());
        menuMap[itemId] = std::string(title + "\t" + menuTitle);
        itemId++;
      }

      AppendMenuA(hMenubar, MF_POPUP, (UINT_PTR) hMenu, menuTitle.c_str());
    }

    MENUINFO Info;
    Info.cbSize = sizeof(Info);
    Info.fMask = MIM_BACKGROUND | MFT_OWNERDRAW;
    Info.hbrBack = bgBrush;
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
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    // @TODO(): provide impl
  }

  void Window::closeContextMenu() {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu(const std::string &seq) {
    // @TODO(jwerle)
  }

  void Window::setContextMenu (const std::string& seq, const std::string& value) {
    HMENU hPopupMenu = CreatePopupMenu();

    auto menuItems = split(value, '_');
    int index = 1;
    std::vector<std::string> lookup;
    lookup.push_back("");

    for (auto item : menuItems) {
      auto pair = split(trim(item), ':');
      auto key = std::string("");

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
    this->eval(resolveMenuSelection(seq, lookup.at(selection), "contextMenu"));
  }

  int Window::openExternal (const std::string& url) {
    ShellExecute(nullptr, "Open", url .c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    // TODO how to detect success here. do we care?
    return 0;
  }

  void Window::setBackgroundColor(int r, int g, int b, float a) {
    SetBkColor(GetDC(window), RGB(r, g, b));
    app.wcex.hbrBackground = CreateSolidBrush(RGB(r, g, b));

    // this->window->SetBkColor(RGB(r, g, b));
    // SetTextColor( RGB( 220,220,220 ));
  }

  void Window::openDialog (
      const std::string& seq,
      bool isSave,
      bool allowDirs,
      bool allowFiles,
      bool allowMultiple,
      const std::string& defaultPath,
      const std::string& title,
      const std::string& defaultName)
  {
    std::vector<std::string> result_paths;
    std::string result_string = "";
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
        //CLSCTX_INPROC_SERVER,
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
        std::wstring(normalizedDefaultPath.begin(), normalizedDefaultPath.end()).c_str(),
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
          std::wstring(title.begin(), title.end()).c_str()
        );
      } else {
        result = dialog.open->SetTitle(
          std::wstring(title.begin(), title.end()).c_str()
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
          std::wstring(defaultName.begin(), defaultName.end()).c_str()
        );
      } else {
        result = dialog.open->SetFileName(
          std::wstring(defaultName.begin(), defaultName.end()).c_str()
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

      result_paths.push_back(WStringToString(std::wstring(buf)));
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

        result_paths.push_back(WStringToString(std::wstring(buf)));
        path->Release();
        CoTaskMemFree(buf);
      }
    }

      for (size_t i = 0, i_end = result_paths.size(); i < i_end; ++i) {
        result_string += (i ? "\\n" : "");
        std::replace(result_paths[i].begin(), result_paths[i].end(), '\\', '/');
        result_string += result_paths[i];
      }

    auto wrapped_result_string =  std::string("\"" + result_string + "\"");
    this->eval(resolveToRenderProcess(seq, "0", encodeURIComponent(wrapped_result_string)));

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

        std::string meta(w->menuMap[wParam]);
        auto parts = split(meta, '\t');

        if (parts.size() > 1) {
          auto title = parts[0];
          auto parent = parts[1];

          if (std::string(title).find("About") == 0) {
            w->about();
            break;
          }

          if (std::string(title).find("Quit") == 0) {
            w->exit(0);
            break;
          }

          w->eval(resolveMenuSelection("0", title, parent));
        }

        break;
      }

      /* case WM_THEMECHANGED: {
        // alert("theme changed");
        if (::IsThemeActive())
          SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
        else
          SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);
        return 0;
      } */

      /* case WM_CTLCOLORDLG:
      case WM_CTLCOLORSTATIC: {
        if (shouldSystemUseDarkMode()) {
          // HDC hdc = reinterpret_cast<HDC>(wParam);
          // SetTextColor(hdc, darkTextColor);
          // static auto testBrush = CreateSolidBrush(RGB(0, 0, 0));
          // SetBkColor(hdc, RGB(0, 0, 0));

          // if (!hbrBkgnd) {
          // hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0));
          // }

          // return reinterpret_cast<INT_PTR>(bgBrush);
        }

        // static auto testBrush = CreateSolidBrush(RGB(0, 0, 0));
        // return (INT_PTR)(testBrush);
        break;
      } */

      // case WM_ERASEBKGND: {
      //  return 1;
      // }

      case WM_SETTINGCHANGE: {
        char* s = (char *) lParam;
        std::string name(s);

        if (name.find("ImmersiveColorSet") != -1) {
          BOOL darkMode = shouldSystemUseDarkMode();

          allowDarkModeForApp(darkMode);

          if (darkMode) {
            SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);

            int aElements[4] = { COLOR_WINDOW, COLOR_ACTIVECAPTION, COLOR_MENUBAR, COLOR_MENU };
            DWORD aNewColors[4] = { RGB(32, 32, 32), RGB(32, 32, 32), RGB(32, 32, 32), RGB(32, 32, 32) };
            SetSysColors(4, aElements, aNewColors);
          } else {
            SetWindowTheme(hWnd, L"Explorer", nullptr);
            int aElements[4] = { COLOR_WINDOW, COLOR_ACTIVECAPTION, COLOR_MENUBAR, COLOR_MENU };
            DWORD aNewColors[4] = { RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255) };
            SetSysColors(4, aElements, aNewColors);
          }

          refreshImmersiveColorPolicyState();

          //SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(mode)));
          /* SetPropW(hWnd, L"UseImmersiveDarkModeColors", (HANDLE)(LONG_PTR)mode);

          WINDOWCOMPOSITIONATTRIBDATA data = {
            WCA_USEDARKMODECOLORS,
            &mode,
            sizeof(mode)
          };

          setWindowCompositionAttribute(hWnd, &data);

          // DwmSetWindowAttribute(hWnd, 19, &mode, sizeof(long));
          // UpdateWindow(hWnd);

          BOOL mode = FALSE;

          if (shouldSystemUseDarkMode && shouldSystemUseDarkMode()) {
            // if (allowDarkModeForApp) allowDarkModeForApp(true);
            //if (refreshImmersiveColorPolicyState) refreshImmersiveColorPolicyState();
            // SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
            mode = TRUE;

            // int aElements[4] = { COLOR_WINDOW, COLOR_ACTIVECAPTION, COLOR_MENUBAR, COLOR_MENU };
            // DWORD aNewColors[4] = { RGB(32, 32, 32), RGB(255, 255, 255), RGB(32, 32, 32), RGB(32, 32, 32) };
            // SetSysColors(4, aElements, aNewColors);
          } else {
            // int aElements[4] = { COLOR_WINDOW, COLOR_ACTIVECAPTION, COLOR_MENUBAR, COLOR_MENU };
            // DWORD aNewColors[4] = { RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 255, 255), RGB(255, 255, 255) };
            // SetSysColors(4, aElements, aNewColors);
          }

          // SetWindowTheme(hWnd, L"Explorer", NULL);

          // SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(mode)));
          // DwmSetWindowAttribute(hWnd, 20, &mode, sizeof(long));
          // WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &mode, sizeof(mode) };
          // setWindowCompositionAttribute(hWnd, &data);
          // UpdateWindow(hWnd);

          // DrawMenuBar(hWnd);
          //
          // HMENU hMenubar = GetMenu(hWnd);
          // SetMenu(hWnd, hMenubar);
          */
        }
        break;
      }

      case WM_CREATE: {
        if (shouldSystemUseDarkMode()) {
          SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
        } else {
          SetWindowTheme(hWnd, L"Explorer", NULL);
          // int aElements[4] = { COLOR_WINDOW, COLOR_ACTIVECAPTION, COLOR_MENUBAR, COLOR_MENU };
          // DWORD aNewColors[4] = { RGB(32, 32, 32), RGB(255, 255, 255), RGB(32, 32, 32), RGB(32, 32, 32) };
          // SetSysColors(4, aElements, aNewColors);
          // allowDarkModeForApp(true);
          // refreshImmersiveColorPolicyState();
          // mode = TRUE;
          // SetWindowTheme(window, L"DarkMode_Explorer", NULL);
        }

        HMENU hMenubar = CreateMenu();
        // MENUINFO Info;
        // Info.cbSize = sizeof(Info);
        // Info.fMask = MIM_BACKGROUND; // | MFT_OWNERDRAW;
        // Info.hbrBack = bgBrush; // (HBRUSH) bgBrush;
        // SetMenuInfo(hMenubar, &Info);

        /* auto brush = (HBRUSH) CreateSolidBrush(RGB(0, 0, 0));

        MENUINFO MenuInfo = {0};
        MenuInfo.cbSize = sizeof(MenuInfo);
        MenuInfo.hbrBack = brush; // Brush you want to draw
        MenuInfo.fMask = MIM_BACKGROUND;
        MenuInfo.dwStyle = MNS_AUTODISMISS; */

        SetMenu(hWnd, hMenubar);
        break;
      }

      case WM_CLOSE: {
        #if DEBUG == 1
          if (w->opts.canExit) {
            fclose(console);
            FreeConsole();
          }
        #endif

        w->close(0);

        break;

        // if (hbrBkgnd) {
        //  DeleteObject(hbrBkgnd);
        //  hbrBkgnd = nullptr;
        // }
      }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
  }

} // namespace SSC
