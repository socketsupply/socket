#if defined(__linux__) && ~!defined(__ANDROID__)

#include <webkit2/webkit-web-extension.h>
#include "../../common.hh"
#include <stdio.h>

using namespace SSC;

static gboolean onWebPageSendRequest (
  WebKitWebPage* page,
  WebKitURIRequest* request,
  WebKitURIResponse* response,
  gpointer userData
) {
  auto uri = String(webkit_uri_request_get_uri(request));
  auto cwd = String((char*) userData);

  if (uri.starts_with("socket:")) {
    if (uri.starts_with("socket:///")) {
      uri = uri.substr(10);
    } else if (uri.starts_with("socket://")) {
      uri = uri.substr(9);
    } else if (uri.starts_with("socket:")) {
      uri = uri.substr(7);
    }

    auto path = fs::path(cwd) / uri;

    if (!fs::exists(fs::status(path))) {
      auto ext = uri.ends_with(".js") ? "" : ".js";
      path = fs::path(cwd) / "socket" / (uri + ext);
    }

    uri = "file://" + path.string();

    webkit_uri_request_set_uri(request, uri.c_str());
  }

  return false;
}

static void onWebPageCreated (
  WebKitWebExtension* extension,
  WebKitWebPage* page,
  gpointer userData
) {
  g_signal_connect(
    page,
    "send-request",
    G_CALLBACK(onWebPageSendRequest),
    userData
  );
}

extern "C" G_MODULE_EXPORT void webkit_web_extension_initialize_with_user_data (
  WebKitWebExtension* extension,
  GVariant* variant
) {
  auto userData = (gpointer) g_variant_get_string(variant, nullptr);
  g_signal_connect(
    extension,
    "page-created",
    G_CALLBACK(onWebPageCreated),
    userData
  );
}
#endif
