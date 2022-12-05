package __BUNDLE_IDENTIFIER__

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.

 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 */
open class MainActivity : WebViewActivity() {
  override open protected val TAG = "Mainctivity"
  open public lateinit var window: Window
  open public lateinit var runtime: Runtime
  open protected val timer = java.util.Timer()

  companion object {
    init {
      System.loadLibrary("socket-runtime")
    }
  }

  fun getRootDirectory (): String {
    return getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
  }

  override fun onCreate (state: android.os.Bundle?) {
    android.util.Log.d(TAG, "onCreate")
    super.onCreate(state)
    this.runtime = Runtime(this, RuntimeConfiguration(
      rootDirectory = getRootDirectory(),
      assetManager = applicationContext.resources.assets,
      evaluateJavascript = { source ->
        webview?.evaluateJavascript(source, null)
      }
    ))

    android.webkit.WebView.setWebContentsDebuggingEnabled(this.runtime.isDebugEnabled())
    this.window = Window(this.runtime, this)
    this.window.load()

    this.timer.schedule(
      kotlin.concurrent.timerTask {
        //core?.apply {
          //android.util.Log.d(TAG, "Expiring old post data")
          //expirePostData()
        //}
      },
      30L * 1024L, // delay
      30L * 1024L //period
    )
  }

  override fun onDestroy () {
    this.timer.apply {
      cancel()
      purge()
    }

    this.runtime.finalize()
    /*
    this.core?.apply {
      freeAllPostData()
      stopEventLoop()
    }
    */

    super.onDestroy()
  }

  override fun onPause () {
    /*
    this.core?.apply {
      stopEventLoop()
      pauseAllPeers()
    }
    */

    super.onPause()
  }

  override fun onResume () {
    /*
    this.core?.apply {
      runEventLoop()
      resumeAllPeers()
    }
    */

    super.onResume()
  }

  override fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    super.onPageStarted(view, url, bitmap)
    val source = this.window.getJavaScriptPreloadSource()
    val webview = this.webview

    webview?.evaluateJavascript(source, null)
  }
}
