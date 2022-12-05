package __BUNDLE_IDENTIFIER__

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.
 */
open class MainActivity : RuntimeActivity() {
  override open protected val TAG = "Mainctivity"
  open public lateinit var window: Window

  companion object {
    init {
      System.loadLibrary("socket-runtime")
    }
  }

  override fun onCreate (state: android.os.Bundle?) {
    android.util.Log.d(TAG, "onCreate")
    super.onCreate(state)
    android.webkit.WebView.setWebContentsDebuggingEnabled(this.runtime.isDebugEnabled())
    this.window = Window(this.runtime, this)
    this.window.load()
  }

  override fun onDestroy () {
    super.onDestroy()
  }

  override fun onPause () {
    super.onPause()
  }

  override fun onResume () {
    super.onResume()
  }
}
