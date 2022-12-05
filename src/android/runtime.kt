// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

/**
 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 * @TODO(jwerle): look into `androidx.appcompat.app.AppCompatActivity`
 */
open class RuntimeActivity : WebViewActivity() {
  override open protected val TAG = "RuntimeActivity"
  open protected val timer = java.util.Timer()
  open public lateinit var runtime: Runtime

  fun getRootDirectory (): String {
    return getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
  }

  override fun onCreate (state: android.os.Bundle?) {
    super.onCreate(state)

    this.runtime = Runtime(this, RuntimeConfiguration(
      rootDirectory = getRootDirectory(),
      assetManager = applicationContext.resources.assets,
      evaluateJavascript = { source ->
        webview?.evaluateJavascript(source, null)
      }
    ))

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
}

interface IRuntimeConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
  val evaluateJavascript: (String) -> Unit
}

data class RuntimeConfiguration (
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager,
  override val evaluateJavascript: (String) -> Unit
) : IRuntimeConfiguration

open class Runtime (
  activity: RuntimeActivity? = null,
  configuration: RuntimeConfiguration
) {
  var pointer = alloc()
  var activity = activity
  val configuration = configuration;

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
    this.activity = null
  }

  fun getPointer (): Long {
    return pointer
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  external fun isDebugEnabled (): Boolean;
}
