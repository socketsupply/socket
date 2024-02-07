package __BUNDLE_IDENTIFIER__

object console {
  val TAG = "Console"
  fun log (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun info (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun debug (string: String) {
    android.util.Log.d(TAG, string)
  }

  fun error (string: String) {
    android.util.Log.e(TAG, string)
  }
}

class PermissionRequest (callback: (Boolean) -> Unit) {
  val id: Int = (0..16384).random().toInt()
  val callback = callback
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
  open lateinit var notificationChannel: android.app.NotificationChannel
  override open lateinit var runtime: Runtime
  override open lateinit var window: Window

  val permissionRequests = mutableListOf<PermissionRequest>()
  val filePicker = WebViewFilePicker(this)

  companion object {
    init {
      System.loadLibrary("socket-runtime")
    }
  }

  fun checkPermission (permission: String): Boolean {
    val status = androidx.core.content.ContextCompat.checkSelfPermission(
      this.applicationContext,
      permission
    )

    if (status == android.content.pm.PackageManager.PERMISSION_GRANTED) {
      return true
    }

    return false
  }

  fun requestPermissions (permissions: Array<String>, callback: (Boolean) -> Unit) {
    val request = PermissionRequest(callback)
    this.permissionRequests.add(request)
    androidx.core.app.ActivityCompat.requestPermissions(
      this,
      permissions,
      request.id
    )
  }

  fun showFileSystemPicker (
    options: WebViewFilePickerOptions,
    callback: (Array<android.net.Uri>) -> Unit
  ) : Boolean {
    val filePicker = this.filePicker
    this.runOnUiThread {
      filePicker.launch(options, callback)
    }
    return true
  }

  override fun onCreate (state: android.os.Bundle?) {
    // called before `super.onCreate()`
    this.supportActionBar?.hide()
    this.getWindow()?.statusBarColor = android.graphics.Color.TRANSPARENT

    super.onCreate(state)

    this.notificationChannel = android.app.NotificationChannel(
      "__BUNDLE_IDENTIFIER__",
      "__BUNDLE_IDENTIFIER__ Notifications",
      android.app.NotificationManager.IMPORTANCE_DEFAULT
    )

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

    this.runtime.setIsEmulator(
      (
        android.os.Build.BRAND.startsWith("generic") &&
        android.os.Build.DEVICE.startsWith("generic")
      ) ||
      android.os.Build.FINGERPRINT.startsWith("generic") ||
      android.os.Build.FINGERPRINT.startsWith("unknown") ||
      android.os.Build.HARDWARE.contains("goldfish") ||
      android.os.Build.HARDWARE.contains("ranchu") ||
      android.os.Build.MODEL.contains("google_sdk") ||
      android.os.Build.MODEL.contains("Emulator") ||
      android.os.Build.MODEL.contains("Android SDK built for x86") ||
      android.os.Build.MANUFACTURER.contains("Genymotion") ||
      android.os.Build.PRODUCT.contains("sdk_google") ||
      android.os.Build.PRODUCT.contains("google_sdk") ||
      android.os.Build.PRODUCT.contains("sdk") ||
      android.os.Build.PRODUCT.contains("sdk_x86") ||
      android.os.Build.PRODUCT.contains("sdk_gphone64_arm64") ||
      android.os.Build.PRODUCT.contains("vbox86p") ||
      android.os.Build.PRODUCT.contains("emulator") ||
      android.os.Build.PRODUCT.contains("simulator")
    )

    this.window = Window(this.runtime, this)

    this.window.load()
    this.runtime.start()

    if (this.runtime.isPermissionAllowed("notifications")) {
      val notificationManager = this.getSystemService(NOTIFICATION_SERVICE) as android.app.NotificationManager
      notificationManager.createNotificationChannel(this.notificationChannel)
    }
  }

  override fun onStart () {
    this.runtime.start()
    super.onStart()

    val window = this.window
    val action: String? = this.intent?.action
    val data: android.net.Uri? = this.intent?.data

    if (action != null && data != null) {
      this.onNewIntent(this.intent)
    }
  }

  override fun onResume () {
    this.runtime.start()
    return super.onResume()
  }

  override fun onPause () {
    this.runtime.stop()
    return super.onPause()
  }

  override fun onStop () {
    this.runtime.stop()
    return super.onStop()
  }

  override fun onDestroy () {
    this.runtime.destroy()
    return super.onDestroy()
  }

  override fun onNewIntent (intent: android.content.Intent) {
    super.onNewIntent(intent)
    val window = this.window
    val action = intent.action
    val data = intent.data
    val id = intent.extras?.getCharSequence("id")?.toString()

    if (action == null) {
      return
    }

    when (action) {
      "android.intent.action.MAIN",
      "android.intent.action.VIEW" -> {
        val scheme = data?.scheme ?: return
        val applicationProtocol = this.runtime.getConfigValue("meta_application_protocol")
        if (
          applicationProtocol.length > 0 &&
          scheme.startsWith(applicationProtocol)
        ) {
          window.bridge.emit("applicationurl", """{
            "url": "$data"
          }""")
        }
      }

      "notification.response.default" -> {
        window.bridge.emit("notificationresponse", """{
          "id": "$id",
          "action": "default"
        }""")
      }

      "notification.response.dismiss" -> {
        window.bridge.emit("notificationresponse", """{
          "id": "$id",
          "action": "dismiss"
        }""")
      }
    }
  }

  override fun onActivityResult (
    requestCode: Int,
    resultCode: Int,
    intent: android.content.Intent?
  ) {
    super.onActivityResult(requestCode, resultCode, intent)
  }

  override fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    super.onPageStarted(view, url, bitmap)
    this.window.onPageStarted(view, url, bitmap)
  }

  override fun onPageFinished (
    view: android.webkit.WebView,
    url: String
  ) {
    super.onPageFinished(view, url)
    this.window.onPageFinished(view, url)
  }

  override fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response:  android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return this.window.onSchemeRequest(request, response, stream)
  }

  override fun onRequestPermissionsResult (
    requestCode: Int,
    permissions: Array<String>,
    grantResults: IntArray
  ) {
    for (request in this.permissionRequests) {
      if (request.id == requestCode) {
        this.permissionRequests.remove(request)
        request.callback(grantResults.all { r ->
          r == android.content.pm.PackageManager.PERMISSION_GRANTED
        })
        break
      }
    }

    var i = 0
    val seen = mutableSetOf<String>()
    for (permission in permissions) {
      val granted = (
        grantResults[i++] == android.content.pm.PackageManager.PERMISSION_GRANTED
      )

      var name = ""
      when (permission) {
        "android.permission.ACCESS_COARSE_LOCATION",
        "android.permission.ACCESS_FINE_LOCATION" -> {
          name = "geolocation"
        }

        "android.permission.POST_NOTIFICATIONS" -> {
          name = "notifications"
        }
      }

      if (seen.contains(name)) {
        continue
      }

      if (name.length == 0) {
        continue
      }

      seen.add(name)

      this.runOnUiThread {
        val state = if (granted) "granted" else "denied"
        window.bridge.emit("permissionchange", """{
          "name": "$name",
          "state": "$state"
        }""")
      }
    }
  }
}
