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
  open lateinit var runtime: Runtime
  open lateinit var window: Window
  open val timer = java.util.Timer()

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
    // called before `super.onCreate()`
    this.supportActionBar?.hide()
    this.getWindow()?.statusBarColor = android.graphics.Color.TRANSPARENT
    this.getWindow()?.decorView?.systemUiVisibility = (
      android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE or
      android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
    )

    super.onCreate(state)

    this.runtime = Runtime(this, RuntimeConfiguration(
      assetManager = this.applicationContext.resources.assets,
      rootDirectory = this.getRootDirectory(),

      exit = { code ->
        console.log("__EXIT_SIGNAL__=${code}")
        this.finishAndRemoveTask()
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
    this.runtime.start()
  }

  override fun onStart () {
    console.log("start")
    this.runtime.start()
    return super.onStart()
  }

  override fun onResume () {
    console.log("resume")
    this.runtime.resume()
    return super.onResume()
  }

  override fun onPause () {
    console.log("pause")
    this.runtime.pause()
    return super.onPause()
  }

  override fun onStop () {
    console.log("stop")
    this.runtime.stop()
    return super.onStop()
  }

  override fun onDestroy () {
    this.timer.apply {
      cancel()
      purge()
    }

    this.runtime.destroy()
    return super.onDestroy()
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

  override fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response:  android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return this.window.onSchemeRequest(request, response, stream)
  }
}
