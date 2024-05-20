// vim: set sw=2:
package socket.runtime.core

import java.lang.ref.WeakReference

import android.content.Context
import android.util.AttributeSet
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView (context: android.content.Context) : android.webkit.WebView(context)

open class WebChromeClient : android.webkit.WebChromeClient() {}
open class WebViewClient : android.webkit.WebViewClient() {
}
