// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IBridgeConfiguration {
  val evaluateJavascript: (String) -> Unit
}

data class BridgeConfiguration(
  override val evaluateJavascript: (String) -> Unit
) : IBridgeConfiguration

open class Bridge(runtime: Runtime, configuration: IBridgeConfiguration) {
  public var pointer = alloc(runtime.pointer)
  public var runtime = WeakReference(runtime)
  public val configuration = configuration

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (runtimePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun route (msg: String, bytes: ByteArray, size: Int): Boolean;
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class UserMessageHandlerBridge (bridge: Bridge) {
  val TAG = "UserMessageHandlerBridge"

  val runtime = bridge.runtime
  val activity = runtime.get()?.activity?.get()

  fun evaluateJavascript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    activity?.runOnUiThread {
      activity.webview?.evaluateJavascript(source, callback)
    }
  }

  fun throwGlobalError (message: String) {
    val source = "throw new Error(\"$message\")"
    evaluateJavascript(source, null)
  }

  @android.webkit.JavascriptInterface
  final fun postMessage (value: String): String? {
    return this.postMessage(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  final fun postMessage (value: String, bytes: ByteArray?): String? {
    // @TODO
    return null
  }
}
