// vim: set sw=2:
package socket.runtime.core

import android.content.Context
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView (context: android.content.Context) : android.webkit.WebView(context)

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebChromeClient
 */
open class WebChromeClient : android.webkit.WebChromeClient() {}

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient : android.webkit.WebViewClient() {}
