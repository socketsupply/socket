// vim: set sw=2:
package socket.runtime.app

import java.lang.Exception
import java.lang.ref.WeakReference

import android.app.Activity
import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.AssetManager
import android.os.Build
import android.os.Bundle
import android.webkit.MimeTypeMap
import android.webkit.WebView

import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.ProcessLifecycleOwner

import socket.runtime.core.console
import socket.runtime.window.WindowManagerActivity

open class AppPermissionRequest (callback: (Boolean) -> Unit) {
  val id: Int = (0..16384).random().toInt()
  val callback = callback
}

/**
 * The `AppActivity` represents the root activity for the application.
 * It is an extended `WindowManagerActivity` that considers application
 * and platform details like notifications, incoming intents, platform
 * permissions, and more.
 */
open class AppActivity : WindowManagerActivity() {
  open protected val TAG = "AppActivity"
  open lateinit var notificationChannel: NotificationChannel

  val permissionRequests = mutableListOf<AppPermissionRequest>()

  open fun getRootDirectory (): String {
    return this.getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
  }

  fun checkPermission (permission: String): Boolean {
    val status = ContextCompat.checkSelfPermission(this.applicationContext, permission)
    return status == PackageManager.PERMISSION_GRANTED
  }

  fun requestPermissions (permissions: Array<String>, callback: (Boolean) -> Unit) {
    val request = AppPermissionRequest(callback)
    this.permissionRequests.add(request)
    ActivityCompat.requestPermissions(
      this,
      permissions,
      request.id
    )
  }

  override fun onCreate (savedInstanceState: Bundle?) {
    this.supportActionBar?.hide()
    this.getWindow()?.statusBarColor = android.graphics.Color.TRANSPARENT

    super.onCreate(savedInstanceState)

    val rootDirectory = this.getRootDirectory()
    val assetManager = this.applicationContext.resources.assets
    val app = App.getInstance()

    this.notificationChannel = NotificationChannel(
      "__BUNDLE_IDENTIFIER__",
      "__BUNDLE_IDENTIFIER__ Notifications",
      NotificationManager.IMPORTANCE_DEFAULT
    )

    app.apply {
      setMimeTypeMap(MimeTypeMap.getSingleton())
      setAssetManager(assetManager)
      setRootDirectory(rootDirectory)
      setBuildInformation(
        Build.BRAND,
        Build.DEVICE,
        Build.FINGERPRINT,
        Build.HARDWARE,
        Build.MODEL,
        Build.MANUFACTURER,
        Build.PRODUCT
      )
    }

    app.onCreateAppActivity(this)

    if (savedInstanceState == null) {
      WebView.setWebContentsDebuggingEnabled(app.isDebugEnabled())

      this.applicationContext
        .getSharedPreferences("WebSettings", Activity.MODE_PRIVATE)
        .edit()
        .apply {
          putString("scheme", "socket")
          putString("hostname", "__BUNDLE_IDENTIFIER__")
          apply()
        }
    }
  }

  override fun onStart () {
    super.onStart()
  }

  override fun onResume () {
    super.onResume()
  }

  override fun onPause () {
    super.onPause()
  }

  override fun onStop () {
    super.onStop()
  }

  override fun onDestroy () {
    super.onDestroy()
    App.getInstance().onDestroyAppActivity(this)
  }

  override fun onNewIntent (intent: Intent) {
    super.onNewIntent(intent)
  }

  override fun onActivityResult (
    requestCode: Int,
    resultCode: Int,
    intent: Intent?
  ) {
    super.onActivityResult(requestCode, resultCode, intent)
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
          r == PackageManager.PERMISSION_GRANTED
        })
        break
      }
    }

    var i = 0
    val seen = mutableSetOf<String>()
    for (permission in permissions) {
      val granted = (
        grantResults[i++] == PackageManager.PERMISSION_GRANTED
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
        App.getInstance().onPermissionChange(name, state)
      }
    }
  }
}

open class AppLifecycleObserver : DefaultLifecycleObserver {
  override fun onDestroy (owner: LifecycleOwner) {
  }

  override fun onStart (owner: LifecycleOwner) {
    App.getInstance().onStart()
  }

  override fun onStop (owner: LifecycleOwner) {
    App.getInstance().onStop()
  }

  override fun onPause (owner: LifecycleOwner) {
    App.getInstance().onPause()
  }

  override fun onResume (owner: LifecycleOwner) {
    App.getInstance().onResume()
  }
}

open class App : Application() {
  val pointer = alloc()

  protected val lifecycleListener: AppLifecycleObserver by lazy {
    AppLifecycleObserver()
  }

  companion object {
    lateinit var appInstance: App

    fun getInstance () : App {
      return appInstance
    }

    fun loadSocketRuntime () {
      System.loadLibrary("socket-runtime")
    }
  }

  init {
    App.Companion.appInstance = this
  }

  override fun onCreate () {
    super.onCreate()

    val lifecycleListener = this.lifecycleListener

    ProcessLifecycleOwner.get().lifecycle.apply {
      addObserver(lifecycleListener)
    }
  }

  @Throws(Exception::class)
  protected external fun alloc (): Long

  @Throws(Exception::class)
  external fun setRootDirectory (rootDirectory: String): Boolean

  @Throws(Exception::class)
  external fun setAssetManager (assetManager: AssetManager): Boolean

  @Throws(Exception::class)
  external fun setMimeTypeMap (mimeTypeMap: MimeTypeMap): Boolean

  @Throws(Exception::class)
  external fun setBuildInformation (
    brand: String,
    device: String,
    fingerprint: String,
    hardware: String,
    model: String,
    manufacturer: String,
    product: String
  ): Unit

  @Throws(Exception::class)
  external fun getUserConfigValue (key: String): String

  @Throws(Exception::class)
  external fun hasRuntimePermission (permission: String): Boolean

  @Throws(Exception::class)
  external fun isDebugEnabled (): Boolean

  @Throws(Exception::class)
  external fun onCreateAppActivity (activity: AppActivity): Unit

  @Throws(Exception::class)
  external fun onDestroyAppActivity (activity: AppActivity): Unit

  @Throws(Exception::class)
  external fun onStart (): Unit

  @Throws(Exception::class)
  external fun onStop (): Unit

  @Throws(Exception::class)
  external fun onResume (): Unit

  @Throws(Exception::class)
  external fun onPause (): Unit

  @Throws(Exception::class)
  external fun onDestroy (): Unit

  @Throws(Exception::class)
  external fun onPermissionChange (name: String, state: String): Unit
}
