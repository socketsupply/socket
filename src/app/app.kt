// vim: set sw=2:
package socket.runtime.app

import java.lang.Exception
import java.lang.ref.WeakReference

import android.app.Activity
import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.ContentUris
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.AssetManager
import android.content.res.AssetFileDescriptor
import android.content.ContentResolver
import android.database.Cursor
import android.graphics.Insets
import android.graphics.drawable.Icon
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.DocumentsContract
import android.provider.MediaStore
import android.view.WindowInsets
import android.view.WindowManager
import android.webkit.MimeTypeMap
import android.webkit.WebView

import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.graphics.drawable.IconCompat
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.ProcessLifecycleOwner

import socket.runtime.core.console
import socket.runtime.window.WindowManagerActivity

import __BUNDLE_IDENTIFIER__.R

open class AppPermissionRequest (callback: (Boolean) -> Unit) {
  val id: Int = (0..16384).random().toInt()
  val callback = callback
}

open class AppPlatform (val activity: AppActivity) {
  fun isDocumentURI (url: String): Boolean {
    return DocumentsContract.isDocumentUri(
      this.activity.applicationContext,
      Uri.parse(url)
    )
  }

  fun getDocumentID (url: String): String {
    return DocumentsContract.getDocumentId(Uri.parse(url))
  }

  fun getContentURI (baseURL: String, documentID: Long): String {
    val uri = ContentUris.withAppendedId(
      Uri.parse(baseURL),
      documentID
    )

    return uri.toString()
  }

  fun getContentMimeType (url: String): String {
    return this.activity.applicationContext.contentResolver.getType(Uri.parse(url)) ?: ""
  }

  fun getPathnameEntriesFromContentURI (url: String): Array<String> {
    val context = this.activity.applicationContext
    val column = MediaStore.MediaColumns.DATA
    var cursor: Cursor? = null
    val uri = Uri.parse(url)
    val results = mutableListOf<String>()

    try {
      cursor = context.contentResolver.query(
        uri,
        arrayOf(column),
        null,
        null,
        null
      )

      if (cursor != null) {
        cursor.moveToFirst()

        do {
          val result = cursor.getString(cursor.getColumnIndex(column))

          if (result != null && result.length > 0) {
            results += result
          } else {
            val tmp = try {
              cursor.getString(cursor.getColumnIndex(MediaStore.MediaColumns._ID))
            } catch (e: Exception) {
              null
            }

            if (tmp != null) {
              results += cursor.getString(cursor.getColumnIndex(column))
            }
          }
        } while (cursor.moveToNext())
      }
    } catch (_: Exception) {
      // not handled
    } finally {
      if (cursor != null) {
        cursor.close()
      }
    }

    return results.toTypedArray()
  }

  fun getPathnameFromContentURIDataColumn (
    url: String,
    id: String
  ): String {
    val context = this.activity.applicationContext
    val column = MediaStore.MediaColumns.DATA
    var cursor: Cursor? = null
    var result: String? = null
    val uri = Uri.parse(url)

    try {
      cursor = context.contentResolver.query(
        uri,
        arrayOf(column),
        null,
        null,
        null
      )

      if (cursor != null) {
        cursor.moveToFirst()
        do {
          if (id.length > 0 && (result == null || result.length == 0)) {
            result = cursor.getString(cursor.getColumnIndex(column))
            break
          } else {
            var index = cursor.getColumnIndex(MediaStore.MediaColumns._ID)
            var tmp: String? = null

            try {
              tmp = cursor.getString(index)
            } catch (e: Exception) {}

            if (tmp == id) {
              index = cursor.getColumnIndex(column)
              result = cursor.getString(index)
              break
            }
          }
        } while (cursor.moveToNext())
      }
    } catch (err: Exception) {
      return ""
    } finally {
      if (cursor != null) {
        cursor.close()
      }
    }

    return result ?: ""
  }

  fun getExternalContentURIForType (type: String): String {
    val contentURI = (
      if (type == "image") {
        MediaStore.Images.Media.EXTERNAL_CONTENT_URI
      } else if (type == "video") {
        MediaStore.Video.Media.EXTERNAL_CONTENT_URI
      } else if (type == "audio") {
        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
      } else {
        MediaStore.Files.getContentUri("external")
      }
    )

    if (contentURI == null) {
      return ""
    }

    return contentURI.toString()
  }

  fun getContentResolver (): ContentResolver {
    return this.activity.getContentResolver()
  }

  fun openContentResolverFileDescriptor (url: String): AssetFileDescriptor? {
    val contentResolver = this.activity.applicationContext.contentResolver
    console.log("url: $url")
    try {
      return contentResolver.openAssetFileDescriptor(Uri.parse(url), "r")
    } catch (err: Exception) {
      console.log("error: $err")
      return null
    }
  }

  fun hasContentResolverAccess (url: String): Boolean {
    val contentResolver = this.activity.applicationContext.contentResolver

    try {
      val fd = contentResolver.openAssetFileDescriptor(Uri.parse(url), "r")

      if (fd == null) {
        return false
      }

      fd.close()
    } catch (_: Exception) {
      return false
    }

    return true
  }
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
  open lateinit var notificationManager: NotificationManager

  open val platform = AppPlatform(this)
  open val permissionRequests = mutableListOf<AppPermissionRequest>()

  open fun getRootDirectory (): String {
    return this.getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
  }

  fun getAppPlatform (): AppPlatform {
    return this.platform
  }

  fun checkPermission (permission: String): Boolean {
    val status = ContextCompat.checkSelfPermission(this.applicationContext, permission)
    return status == PackageManager.PERMISSION_GRANTED
  }

  fun requestPermissions (permissions: Array<String>) {
    return this.requestPermissions(permissions, fun (_: Boolean) {})
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

  fun openExternal (value: String) {
    val uri = Uri.parse(value)
    val action = Intent.ACTION_VIEW
    val intent = Intent(action, uri)
    this.startActivity(intent)
  }

  fun getScreenInsets (): Insets {
    val windowManager = this.applicationContext.getSystemService(
      Context.WINDOW_SERVICE
    ) as WindowManager
    val metrics = windowManager.getCurrentWindowMetrics()
    val windowInsets = metrics.windowInsets
    return windowInsets.getInsetsIgnoringVisibility(
      WindowInsets.Type.navigationBars() or
      WindowInsets.Type.displayCutout()
    )
  }

  fun getScreenSizeWidth (): Int {
    val insets = this.getScreenInsets()
    return insets.right + insets.left
  }

  fun getScreenSizeHeight (): Int {
    val insets = this.getScreenInsets()
    return insets.top + insets.bottom
  }

  fun getAssetManager (): AssetManager {
    return this.applicationContext.resources.assets
  }

  fun isNotificationManagerEnabled (): Boolean {
    return NotificationManagerCompat.from(this).areNotificationsEnabled()
  }

  fun showNotification (
    id: String,
    title: String,
    body: String,
    tag: String,
    channel: String,
    category: String,
    silent: Boolean,
    iconURL: String,
    imageURL: String,
    vibratePattern: String
  ): Boolean {
    // paramters
    val identifier = id.toLongOrNull()?.toInt() ?: (0..16384).random().toInt()
    val vibration = vibratePattern
      .split(",")
      .filter({ it.length > 0 })
      .map({ it.toInt().toLong() })
      .toTypedArray()

    // intents
    val intentFlags = (
      Intent.FLAG_ACTIVITY_SINGLE_TOP
    )

    val contentIntent = Intent(this, __BUNDLE_IDENTIFIER__.MainActivity::class.java)
    val deleteIntent = Intent(this, __BUNDLE_IDENTIFIER__.MainActivity::class.java)

    // TODO(@jwerle): move 'action' to a constant
    contentIntent.addCategory(Intent.CATEGORY_LAUNCHER)
    contentIntent.setAction("notification.response.default")
    contentIntent.putExtra("id", id)
    contentIntent.flags = intentFlags

    // TODO(@jwerle): move 'action' to a constant
    deleteIntent.setAction("notification.response.dismiss")
    deleteIntent.putExtra("id", id)
    deleteIntent.flags = intentFlags

    // pending intents
    val pendingContentIntent: PendingIntent = PendingIntent.getActivity(
      this,
      identifier,
      contentIntent,
      (
        PendingIntent.FLAG_UPDATE_CURRENT or
        PendingIntent.FLAG_IMMUTABLE or
        PendingIntent.FLAG_ONE_SHOT
      )
    )

    val pendingDeleteIntent: PendingIntent = PendingIntent.getActivity(
      this,
      identifier,
      deleteIntent,
      (
        PendingIntent.FLAG_UPDATE_CURRENT or
        PendingIntent.FLAG_IMMUTABLE
      )
    )

    // build notification
    val builder = NotificationCompat.Builder(this, channel).apply {
      setPriority(NotificationCompat.PRIORITY_DEFAULT)
      setContentTitle(title)
      setContentIntent(pendingContentIntent)
      setDeleteIntent(pendingDeleteIntent)
      setAutoCancel(true)
      setSilent(silent)
      if (vibration.size > 0) {
        setVibrate(vibration.toLongArray())
      }

      if (body.length > 0) {
        setContentText(body)
      }
    }

    if (iconURL.length > 0) {
      val icon = IconCompat.createWithContentUri(iconURL)
      builder.setSmallIcon(icon)
    } else {
      val icon = IconCompat.createWithResource(
        this,
        R.mipmap.ic_launcher_round
      )

      builder.setSmallIcon(icon)
    }

    if (imageURL.length > 0) {
      val icon = Icon.createWithContentUri(imageURL)
      builder.setLargeIcon(icon)
    }

    if (category.length > 0) {
      builder.setCategory(
        category
          .replace("msg", "message")
          .replace("-", "_")
      )
    }

    val notification = builder.build()
    with (NotificationManagerCompat.from(this)) {
      notify(
        tag,
        identifier,
        notification
      )
    }

    return true
  }

  fun closeNotification (id: String, tag: String): Boolean {
    val identifier = id.toLongOrNull()?.toInt() ?: (0..16384).random().toInt()
    with (NotificationManagerCompat.from(this)) {
      cancel(tag, identifier)
    }
    return true
  }

  override fun onCreate (savedInstanceState: Bundle?) {
    this.supportActionBar?.hide()
    this.getWindow()?.statusBarColor = android.graphics.Color.TRANSPARENT

    super.onCreate(savedInstanceState)

    val externalStorageDirectory = Environment.getExternalStorageDirectory().absolutePath
    val externalFilesDirectory = this.getExternalFilesDir(null)?.absolutePath ?: "$externalStorageDirectory/Desktop"
    val cacheDirectory = this.applicationContext.getCacheDir().absolutePath
    val rootDirectory = this.getRootDirectory()
    val assetManager = this.applicationContext.resources.assets
    val app = App.getInstance()

    this.notificationChannel = NotificationChannel(
      "__BUNDLE_IDENTIFIER__",
      "__BUNDLE_IDENTIFIER__ Notifications",
      NotificationManager.IMPORTANCE_DEFAULT
    )

    this.notificationManager = this.getSystemService(NOTIFICATION_SERVICE) as NotificationManager

    app.apply {
      setMimeTypeMap(MimeTypeMap.getSingleton())
      setAssetManager(assetManager)

      // directories
      setRootDirectory(rootDirectory)
      setExternalCacheDirectory(cacheDirectory)
      setExternalFilesDirectory(externalFilesDirectory)
      setExternalStorageDirectory(externalStorageDirectory)

      // build info
      setBuildInformation(
        Build.BRAND,
        Build.DEVICE,
        Build.FINGERPRINT,
        Build.HARDWARE,
        Build.MODEL,
        Build.MANUFACTURER,
        Build.PRODUCT
      )

      setWellKnownDirectories(
        // 'Downloads/'
        this.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)?.absolutePath ?: "",
        // 'Documents/'
        this.getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS)?.absolutePath ?: "",
        // 'Pictures/'
        this.getExternalFilesDir(Environment.DIRECTORY_PICTURES)?.absolutePath ?: "",
        // 'Desktop/'
        externalFilesDirectory,
        // 'Videos/'
        this.getExternalFilesDir(Environment.DIRECTORY_DCIM)?.absolutePath ?: "",
        // "configuration directory"
        externalFilesDirectory,
        // "media directory"
        externalStorageDirectory + "Android/media/__BUNDLE_IDENTIFIER__",
        // 'Music/'
        this.getExternalFilesDir(Environment.DIRECTORY_MUSIC)?.absolutePath ?: "",
        // '~/'
        externalStorageDirectory,
        // "data directory"
        externalFilesDirectory,
        // "logs directory"
        externalFilesDirectory,
        // 'tmp/' (likely the cache directory will be used instead of this value)
        externalStorageDirectory + "/tmp/__BUNDLE_IDENTIFIER__"
      )
    }

    app.onCreateAppActivity(this)

    if (app.hasRuntimePermission("notifications")) {
      this.notificationManager.createNotificationChannel(this.notificationChannel)
    }

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
    val action: String? = this.intent?.action
    val data: android.net.Uri? = this.intent?.data

    if (action != null && data != null) {
      this.onNewIntent(this.intent)
    }
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
    val action = intent.action
    val data = intent.data
    val id = intent.extras?.getCharSequence("id")?.toString()

    when (action) {
      "android.intent.action.MAIN",
      "android.intent.action.VIEW" -> {
        val scheme = data?.scheme ?: return
        val applicationProtocol = App.getInstance().getUserConfigValue("meta_application_protocol")
        if (
          applicationProtocol.length > 0 &&
          scheme.startsWith(applicationProtocol)
        ) {
          for (fragment in this.windowFragmentManager.fragments) {
            val window = fragment.window ?: continue
            window.handleApplicationURL(data.toString())
          }
        }
      }

      "notification.response.default" -> {
        for (fragment in this.windowFragmentManager.fragments) {
          val bridge = fragment.window?.bridge ?: continue
          bridge.emit("notificationresponse", """{
            "id": "$id",
            "action": "default"
          }""")
        }
      }

      "notification.response.dismiss" -> {
        for (fragment in this.windowFragmentManager.fragments) {
          val bridge = fragment.window?.bridge ?: continue
          bridge.emit("notificationresponse", """{
            "id": "$id",
            "action": "dismiss"
          }""")
        }
      }
    }
  }

  override fun onRequestPermissionsResult (
    requestCode: Int,
    permissions: Array<String>,
    grantResults: IntArray
  ) {
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

        "android.permission.CAMERA" -> {
          name = "camera"
        }

        "android.permission.RECORD_AUDIO" -> {
          name = "microphone"
        }
      }

      if (seen.contains(name)) {
        continue
      }

      if (name.length == 0) {
        continue
      }

      seen.add(name)

      val state = if (granted) "granted" else "denied"
      App.getInstance().onPermissionChange(name, state)
    }

    for (request in this.permissionRequests) {
      if (request.id == requestCode) {
        this.permissionRequests.remove(request)
        request.callback(grantResults.all { r ->
          r == PackageManager.PERMISSION_GRANTED
        })
        return
      }
    }

    super.onRequestPermissionsResult(requestCode, permissions, grantResults)
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
  external fun setExternalCacheDirectory (cacheDirectory: String): Boolean

  @Throws(Exception::class)
  external fun setExternalStorageDirectory (directory: String): Boolean

  @Throws(Exception::class)
  external fun setExternalFilesDirectory (directory: String): Boolean

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
  external fun setWellKnownDirectories (
    downloads: String = "",
    documents: String = "",
    pictures: String = "",
    desktop: String = "",
    videos: String = "",
    config: String = "",
    media: String = "",
    music: String = "",
    home: String = "",
    data: String = "",
    log: String = "",
    tmp: String = ""
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
