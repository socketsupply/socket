package __BUNDLE_IDENTIFIER__

object console {
  val TAG = "Console"
  fun log (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun debug (string: String) {
    android.util.Log.d(TAG, string)
  }

  fun error (string: String) {
    android.util.Log.e(TAG, string)
  }
}

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.

 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 */
open class MainActivity : WebViewActivity() {
  override open protected val TAG = "Mainctivity"
  open public lateinit var runtime: Runtime
  open public lateinit var window: Window
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
    super.onCreate(state)

    this.runtime = Runtime(this, RuntimeConfiguration(
      assetManager = this.applicationContext.resources.assets,
      rootDirectory = this.getRootDirectory(),

      exit = { code ->
        console.log("__EXIT_SIGNAL__=${code}")
        this.finishAndRemoveTask()
      },

      evaluateJavascript = { source ->
        this.webview?.evaluateJavascript(source, null)
      },

      openExternal = { value ->
        val uri = android.net.Uri.parse(value)
        val action = android.content.Intent.ACTION_VIEW
        val intent = android.content.Intent(action, uri)
        this.startActivity(intent)
      }
    ))

    this.window = Window(this.runtime, this)

    this.timer.schedule(
      kotlin.concurrent.timerTask {
        // TODO
      },
      30L * 1024L, // delay
      30L * 1024L //period
    )

    this.window.load()
    //this.runtime.start()
  }

  override fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response:  android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return this.window.onSchemeRequest(request, response, stream)
  }

  override fun onDestroy () {
    this.timer.apply {
      cancel()
      purge()
    }

    this.runtime.destroy()
    return super.onDestroy()
  }

  override fun onPause () {
    this.runtime.pause()
    return super.onPause()
  }

  override fun onResume () {
    this.runtime.resume()
    return super.onResume()
  }

  override fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    val source = this.window.getJavaScriptPreloadSource()
    this.webview?.evaluateJavascript(source, null)
    return super.onPageStarted(view, url, bitmap)
  }
}
