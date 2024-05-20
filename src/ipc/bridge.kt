// vim: set sw=2:
package socket.runtime.ipc

import android.content.Intent
import android.webkit.WebView
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse

import androidx.appcompat.app.AppCompatActivity

import socket.runtime.app.App
import socket.runtime.ipc.Navigator
import socket.runtime.ipc.SchemeHandlers
import socket.runtime.core.console
import socket.runtime.core.WebViewClient

private fun isAndroidAssetsUri (uri: android.net.Uri): Boolean {
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

open class Bridge (val index: Int, val activity: AppCompatActivity): WebViewClient() {
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

    val allowed = this.navigator.isNavigationRequestAllowed(
      view.url ?: "",
      request.url.toString()
    )

    if (allowed) {
      console.log("is allowed")
      return false
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
}
