// vim: set sw=2:
package socket.runtime.ipc

import android.content.Intent
import android.graphics.Bitmap
import android.net.Uri
import android.webkit.WebView
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse

import socket.runtime.app.App
import socket.runtime.core.console
import socket.runtime.core.WebViewClient
import socket.runtime.ipc.Navigator
import socket.runtime.ipc.SchemeHandlers
import socket.runtime.window.Window
import socket.runtime.window.WindowManagerActivity

private fun isAndroidAssetsUri (uri: Uri): Boolean {
  if (uri.pathSegments.size == 0) {
    return false
  }

  val scheme = uri.scheme
  val host = uri.host
  // handle no path segments, not currently required but future proofing
  val path = uri.pathSegments?.get(0)

  if (host == "appassets.androidplatform.net") {
    return true
  }

  if (scheme == "file" && host == "" && path == "android_asset") {
    return true
  }

  return false
}

open class Bridge (
  val index: Int,
  val activity: WindowManagerActivity,
  val window: Window
): WebViewClient() {
  open val schemeHandlers = SchemeHandlers(this)
  open val navigator = Navigator(this)
  open val buffers = mutableMapOf<String, ByteArray>()

  override fun shouldOverrideUrlLoading (
    view: WebView,
    request: WebResourceRequest
  ): Boolean {
    if (isAndroidAssetsUri(request.url)) {
      return false
    }

    val app = App.getInstance()
    val bundleIdentifier = app.getUserConfigValue("meta_bundle_identifier")

    if (request.url.host == bundleIdentifier) {
      return false
    }

    if (
      request.url.scheme == "ipc" ||
      request.url.scheme == "node" ||
      request.url.scheme == "npm" ||
      request.url.scheme == "socket"
    ) {
      return false
    }

    val scheme = request.url.scheme
    if (scheme != null && schemeHandlers.hasHandlerForScheme(scheme)) {
      return true
    }

    val allowed = this.navigator.isNavigationRequestAllowed(
      view.url ?: "",
      request.url.toString()
    )

    if (allowed) {
      return true
    }

    val intent = Intent(Intent.ACTION_VIEW, request.url)

    try {
      this.activity.startActivity(intent)
    } catch (err: Exception) {
      // TODO(jwerle): handle this error gracefully
      console.error(err.toString())
      return false
    }

    return true
  }

  override fun shouldInterceptRequest (
    view: WebView,
    request: WebResourceRequest
  ): WebResourceResponse? {
    return this.schemeHandlers.handleRequest(request)
  }

  override fun onPageStarted (
    view: WebView,
    url: String,
    favicon: Bitmap?
  ) {
    val uri = Uri.parse(url)
    if (uri.authority != "__BUNDLE_IDENTIFIER__") {
      val preloadUserScript = this.window.getPreloadUserScript()
      view.evaluateJavascript(preloadUserScript, { _ -> })
    }

    super.onPageStarted(view, url, favicon)
  }

  fun emit (event: String, data: String): Boolean {
    return this.emit(this.index, event, data)
  }

  @Throws(Exception::class)
  external fun emit (index: Int, event: String, data: String): Boolean
}
