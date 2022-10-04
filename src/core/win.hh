#ifndef SSC_CORE_WIN_H
#define SSC_CORE_WIN_H
#if defined(_WIN32)
#include <WebView2.h>
#include <WebView2Experimental.h>
#include <WebView2EnvironmentOptions.h>
#include <WebView2ExperimentalEnvironmentOptions.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")
#pragma comment(lib, "uv_a.lib")
#endif
#endif
