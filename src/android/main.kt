package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference
import java.util.Timer

object console {
  val TAG = "Console"
  fun log (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun warn (string: String) {
    android.util.Log.w(TAG, string)
  }

  fun debug (string: String) {
    android.util.Log.d(TAG, string)
  }

  fun error (string: String) {
    android.util.Log.e(TAG, string)
  }
}

open class BroadcastReceiver (
  activity: MainActivity
): android.content.BroadcastReceiver() {
  val activity = WeakReference(activity)

  override fun onReceive (
    context: android.content.Context,
    intent: android.content.Intent
  ) {
    if (intent.action == BluetoothDevice.ACTION_PAIRING_REQUEST) {
      val device: BluetoothDevice? = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
      console.log("ACTION_PAIRING_REQUEST")

      if (device == null) {
        return
      }

      try {
        device.setPin(Bluetooth.PIN_DEFAULT_VALUE.toByteArray())
        activity.get()?.bluetooth?.handler?.postDelayed({ device.createBond() }, 32L)
      } catch (err: Exception) {
        console.log("ACTION_PAIRING_REQUEST error: ${err.toString()}")
      }
    }
  }
}

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `socket.ini` for
 * advanced usage.

 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 */
open class MainActivity : WebViewActivity() {
  override open protected val TAG = "Mainctivity"

  open lateinit var bluetooth: Bluetooth
  open lateinit var runtime: Runtime
  open lateinit var window: Window

  companion object {
    init {
      System.loadLibrary("socket-runtime")
    }
  }

  fun getRootDirectory (): String {
    return this.getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
  }

  fun getRequestedPermissions (context: android.content.Context): Array<String> {
    try {
      val info = context.packageManager.getPackageInfo(
        context.getPackageName(),
        android.content.pm.PackageManager.GET_PERMISSIONS
      )

      return info.requestedPermissions
    } catch (err: android.content.pm.PackageManager.NameNotFoundException) {
      console.warn("getRequestedPermissions failed: ${err.toString()}")
      return emptyArray<String>()
    }
  }

  override fun onCreate (state: android.os.Bundle?) {
    // called before `super.onCreate()`
    this.supportActionBar?.hide()
    this.getWindow()?.statusBarColor = android.graphics.Color.TRANSPARENT

    // called after supportActionBar and statusBarColor are modified
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

    this.bluetooth = Bluetooth(this.getApplicationContext(), this.runtime, BluetoothConfiguration(
      manager = this.getSystemService(
        android.bluetooth.BluetoothManager::class.java,
      ),

      emit = { data, err ->
        this.window.bridge.emitBluetoothEventData(data, err ?: "")
      },

      send = { serviceId, characteristicId, name, uuid, bytes->
        this.window.bridge.sendBluetoothByteArray(
          serviceId,
          characteristicId,
          name,
          uuid,
          bytes
        )
      }
    ))

    this.window = Window(this.runtime, this)
    this.window.load()
    this.runtime.start()

    this.registerReceiver(
      BroadcastReceiver(this),
      android.content.IntentFilter(BluetoothDevice.ACTION_PAIRING_REQUEST)
    )

    if (this.bluetooth.isSupported()) {
      if (this.bluetooth.isEnabled()) {
        val requestedPermissions = this.getRequestedPermissions(this.applicationContext)
        val requiredPermissions = arrayOf(
          android.Manifest.permission.ACCESS_COARSE_LOCATION,
          android.Manifest.permission.BLUETOOTH,
          android.Manifest.permission.BLUETOOTH_SCAN,
          android.Manifest.permission.BLUETOOTH_ADMIN,
          android.Manifest.permission.BLUETOOTH_CONNECT,
          android.Manifest.permission.BLUETOOTH_ADVERTISE
        )

        var hasAllRequiredPermissions = true
        for (permission in requiredPermissions) {
          if (!requestedPermissions.contains(permission)) {
            hasAllRequiredPermissions = false
            break
          }
        }

        if (hasAllRequiredPermissions) {
          try {
            androidx.core.app.ActivityCompat.requestPermissions(
              this,
              requiredPermissions,
              1
            )
          } catch (err: Exception) {
            console.error(
              "Failed to request permissions needed for bluetooth: ${err.toString()}"
            )
          }
        }
      } else {
        val action = android.bluetooth.BluetoothAdapter.ACTION_REQUEST_ENABLE
        val intent = android.content.Intent(action)
        this.startActivityForResult(intent, Bluetooth.REQUEST_ENABLE_BLUETOOTH)
      }
    }
  }

  override fun onStart () {
    super.onStart()
    this.runtime.start()
  }

  override fun onResume () {
    super.onResume()
    this.runtime.resume()
  }

  override fun onPause () {
    super.onPause()
    this.runtime.pause()
  }

  override fun onStop () {
    super.onStop()
    this.runtime.stop()
  }

  override fun onDestroy () {
    super.onDestroy()
    this.runtime.destroy()
  }

  override fun onRequestPermissionsResult (
    requestCode: Int,
    permissions: Array<String>,
    grants: IntArray
  ) {
    super.onRequestPermissionsResult(requestCode, permissions, grants)

    for (i in permissions.indices) {
      val permission = permissions[i]
      val grant = grants[i]

      if (
        permission == android.Manifest.permission.BLUETOOTH &&
        grant == android.content.pm.PackageManager.PERMISSION_GRANTED
      ) {
        this.bluetooth.start()
      }

      if (grant != android.content.pm.PackageManager.PERMISSION_GRANTED) {
        console.warn("permission '$permission' was not granted with: '$grant'")
      }
    }
  }

  override fun onActivityResult (
    requestCode: Int,
    resultCode: Int,
    data: android.content.Intent?
  ) {
    super.onActivityResult(requestCode, resultCode, data)

    if (requestCode == Bluetooth.REQUEST_ENABLE_BLUETOOTH) {
      if (requestCode == RESULT_OK) {
        val permissions = arrayOf(
          android.Manifest.permission.BLUETOOTH,
          android.Manifest.permission.BLUETOOTH_SCAN,
          android.Manifest.permission.BLUETOOTH_CONNECT,
          android.Manifest.permission.BLUETOOTH_ADVERTISE
        )

        androidx.core.app.ActivityCompat.requestPermissions(this, permissions, 1)
      } else {
        console.error("Failed to enable bluetooth in MainActivity")
      }
    }
  }

  override fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    super.onPageStarted(view, url, bitmap)
    val source = this.window.getJavaScriptPreloadSource()
    this.webview?.evaluateJavascript(source, null)
  }

  override fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response:  android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return this.window.onSchemeRequest(request, response, stream)
  }
}
