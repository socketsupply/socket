#ifndef SSC_CORE_WIN_H
#define SSC_CORE_WIN_H
#if defined(_WIN32)
#include <WebView2/WebView2.h>
#include <WebView2/WebView2EnvironmentOptions.h>
#pragma comment(lib, "WebView2LoaderStatic.lib")
#endif
#endif
