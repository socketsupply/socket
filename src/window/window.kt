// vim: set sw=2:
package socket.runtime.window

import java.lang.ref.WeakReference

import kotlin.concurrent.thread

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.webkit.WebSettings
import android.webkit.WebView
import android.widget.FrameLayout
import android.webkit.GeolocationPermissions
import android.webkit.PermissionRequest

import androidx.appcompat.app.AppCompatActivity
import androidx.core.os.bundleOf
import androidx.fragment.app.Fragment
import androidx.fragment.app.commit

import socket.runtime.app.App
import socket.runtime.core.console
import socket.runtime.core.WebChromeClient
import socket.runtime.ipc.Bridge
import socket.runtime.ipc.Message
import socket.runtime.window.WindowFragment
import socket.runtime.window.Dialog

import __BUNDLE_IDENTIFIER__.R

/**
 */
data class Size (val width: Int, val height: Int);

/**
 */
data class ScreenSize (val width: Int, val height: Int);

/**
 * A container for configuring a `Window`
 */
data class WindowOptions (
  var index: Int = 0,
  var shouldExitApplicationOnClose: Boolean = false
)

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class WindowWebViewUserMessageHandler (window: Window) {
  val TAG = "WindowWebViewUserMessageHandler"

  val namespace = "external"
  val window = window

  @android.webkit.JavascriptInterface
  open fun postMessage (value: String): Boolean {
    return this.postMessage(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  open fun postMessage (value: String, bytes: ByteArray? = null): Boolean {
    val message = Message(value)

    if (message.seq.length > 0 && bytes != null) {
      this.window.bridge.buffers[message.seq] = bytes
    }

    this.window.onMessage(this.window.index, value)
    return true
  }
}

/**
 */
open class WindowWebChromeClient (val window: Window) : WebChromeClient() {
  override fun onGeolocationPermissionsShowPrompt (
    origin: String,
    callback: GeolocationPermissions.Callback
  ) {
    val app = App.getInstance()
    val allowed = app.hasRuntimePermission("geolocation")
    callback(origin, allowed, allowed)
  }

  override fun onPermissionRequest (request: PermissionRequest) {
    val resources = request.resources
    var grants = mutableListOf<String>()
    val app = App.getInstance()

    for (resource in resources) {
      when (resource) {
        PermissionRequest.RESOURCE_AUDIO_CAPTURE -> {
          if (app.hasRuntimePermission("microphone") || app.hasRuntimePermission("user_media")) {
            grants.add(PermissionRequest.RESOURCE_AUDIO_CAPTURE)
          }
        }

        PermissionRequest.RESOURCE_VIDEO_CAPTURE -> {
          if (app.hasRuntimePermission("camera") || app.hasRuntimePermission("user_media")) {
            grants.add(PermissionRequest.RESOURCE_VIDEO_CAPTURE)
          }
        }

        // auto grant EME
        PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID -> {
          grants.add(PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID)
        }
      }
    }

    if (grants.size > 0) {
      request.grant(grants.toTypedArray())
    } else {
      request.deny()
    }
  }

  override fun onShowFileChooser (
    webview: android.webkit.WebView,
    callback: android.webkit.ValueCallback<Array<android.net.Uri>>,
    params: android.webkit.WebChromeClient.FileChooserParams
  ): Boolean {
    val fragment = this.window.fragment
    val activity = fragment.requireActivity() as AppCompatActivity
    return true
  }
}

/**
 */
open class Window (val fragment: WindowFragment) {
  val userMessageHandler = WindowWebViewUserMessageHandler(this)
  val activity = fragment.requireActivity()
  val bridge = Bridge(fragment.index, activity as AppCompatActivity)
  val client = WindowWebChromeClient(this)
  val dialog = Dialog(this)
  val index = fragment.index
  var title = ""

  init {
    val userMessageHandler = this.userMessageHandler
    val bridge = this.bridge
    val client = this.client

    fragment.webview.apply {
      // clients
      webViewClient = bridge
      webChromeClient = client

      // external interface
      addJavascriptInterface(userMessageHandler, "external")
    }

    thread {
      val pendingNavigationLocation = this.getPendingNavigationLocation(this.index)
      if (pendingNavigationLocation.length > 0) {
        this.navigate(pendingNavigationLocation)
      }
    }
  }

  fun show () {
    val fragment = this.fragment
    val activity = fragment.activity
    val manager = activity?.supportFragmentManager
    activity?.runOnUiThread {
      manager?.commit {
        show(fragment)
      }
    }
  }

  fun hide () {
    val fragment = this.fragment
    val activity = fragment.activity
    val manager = activity?.supportFragmentManager
    activity?.runOnUiThread {
      manager?.commit {
        hide(fragment)
      }
    }
  }

  fun close () {
  }

  fun navigate (url: String) {
    val fragment = this.fragment
    val activity = fragment.activity
    val webview = fragment.webview
    activity?.runOnUiThread {
      webview.loadUrl(url.replace("socket:", "https:"))
    }
  }

  fun getSize (): Size {
    val webview = this.fragment.webview
    return Size(webview.width, webview.height)
  }

  fun setSize (size: Size) {
    val webview = this.fragment.webview
    val layout = FrameLayout.LayoutParams(size.width, size.height)
    webview.setLayoutParams(layout)
  }

  @Throws(Exception::class)
  external fun onMessage (index: Int, value: String, bytes: ByteArray? = null): Unit

  @Throws(Exception::class)
  external fun getPendingNavigationLocation (index: Int): String
}

/**
 * A fragment that contains a single `WebView`. A `WindowFragment` is managed
 * by a `WindowFragmentManager` and holds a reference to a `SSC::ManagedWindow`
 * instance managed by `SSC::WindowManager` in the native runtime.
 */
open class WindowFragment : Fragment(R.layout.web_view) {
  companion object {
    /**
     * Factory for creating new `WindowFragment` instances from a `Bundle`
     */
    fun newInstance (bundle: Bundle): WindowFragment {
      return WindowFragment().apply {
        arguments = bundle
      }
    }

    /**
     * Factory for creating new `WindowFragment` instances from a `WindowOptions`
     */
    fun newInstance (options: WindowOptions): WindowFragment {
      return WindowFragment().apply {
        arguments = bundleOf(
          "index" to options.index,
          "shouldExitApplicationOnClose" to options.shouldExitApplicationOnClose
        )
      }
    }
  }

  /**
   * The options used to create this `WindowFragment`.
   */
  open val options = WindowOptions()

  /**
   */
  open var window: Window? = null

  /**
   */
  open val index: Int get () = this.options.index

  /**
   * A reference to the child `WebView` owned by this `WindowFragment`
   */
  open lateinit var webview: WebView

  override fun onAttach (context: Context) {
    super.onAttach(context)
    console.log("onAttach")
  }

  override fun onCreate (savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    console.log("onCreate")
  }

  override fun onCreateView (
    inflater: LayoutInflater,
    container: ViewGroup?,
    savedInstanceState: Bundle?
  ): View? {
    console.log("onCreateView")
    return super.onCreateView(inflater, container, savedInstanceState)
  }

  override fun onStart () {
    super.onStart()
    console.log("onStart")
  }

  override fun onStop () {
    super.onStop()
    console.log("onStop")
  }

  override fun onResume () {
    super.onResume()
    console.log("onResume")
  }

  override fun onPause () {
    super.onPause()
    console.log("onPause")
  }

  /**
   * Called when the view was created and when the `WindowFragment` should
   * assign the `WebView` instance.
   */
  override fun onViewCreated (view: View, savedInstanceState: Bundle?) {
    super.onViewCreated(view, savedInstanceState)

    val arguments = this.arguments
    val app = App.getInstance()

    if (arguments != null) {
      this.options.index = arguments.getInt("index", this.options.index)
      this.options.shouldExitApplicationOnClose = arguments.getBoolean(
        "shouldExitApplicationOnClose",
        this.options.shouldExitApplicationOnClose
      )
    }

    this.webview = view.findViewById<WebView>(R.id.webview)

    this.webview.apply {
      // features
      settings.javaScriptEnabled = true

      settings.domStorageEnabled = app.hasRuntimePermission("data_access")
      settings.databaseEnabled = app.hasRuntimePermission("data_access")

      settings.setGeolocationEnabled(app.hasRuntimePermission("geolocation"))
      settings.javaScriptCanOpenWindowsAutomatically = true

      // allow list
      settings.allowFileAccess = true
      settings.allowContentAccess = true

      // allow mixed content
      settings.mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
    }

    this.window = Window(this)
  }
}
