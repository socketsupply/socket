// vim: set sw=2:
package __BUNDLE_IDENTIFIER__

interface IBridgeConfiguration {
  val getRootDirectory: () -> String
}

data class BridgeConfiguration (
  override val getRootDirectory: () -> String
) : IBridgeConfiguration

data class Result (
  val id: Long,
  val seq: String,
  val source: String,
  val value: String,
  val bytes: ByteArray? = null,
  val headers: Map<String, String> = emptyMap()
)

typealias RouteCallback = (Result) -> Unit

data class RouteRequest (
  val id: Long,
  val callback: RouteCallback
)

// container for a parseable IPC message (ipc://...)
class Message (message: String? = null) {
  var uri: android.net.Uri? =
    if (message != null) {
      android.net.Uri.parse(message)
    } else {
      android.net.Uri.parse("ipc://")
    }

  var command: String
    get () = uri?.host ?: ""
    set (command) {
      uri = uri?.buildUpon()?.authority(command)?.build()
    }

  var domain: String
    get () {
      val parts = command.split(".")
      return parts.slice(0..(parts.size - 2)).joinToString(".")
    }
    set (_) {}

  var value: String
    get () = get("value")
    set (value) {
      set("value", value)
    }

  var seq: String
    get () = get("seq")
    set (seq) {
      set("seq", seq)
    }

  var bytes: ByteArray? = null

  fun get (key: String, defaultValue: String = ""): String {
    val value = uri?.getQueryParameter(key)

    if (value != null && value.isNotEmpty()) {
      return value
    }

    return defaultValue
  }

  fun has (key: String): Boolean {
    return get(key).isNotEmpty()
  }

  fun set (key: String, value: String): Boolean {
    uri = uri?.buildUpon()?.appendQueryParameter(key, value)?.build()
    return uri == null
  }

  fun delete (key: String): Boolean {
    if (uri?.getQueryParameter(key) == null) {
      return false
    }

    val params = uri?.queryParameterNames
    val tmp = uri?.buildUpon()?.clearQuery()

    if (params != null) {
      for (param: String in params) {
        if (!param.equals(key)) {
          val value = uri?.getQueryParameter(param)
          tmp?.appendQueryParameter(param, value)
        }
      }
    }

    uri = tmp?.build()

    return true
  }

  override fun toString(): String {
    return uri?.toString() ?: ""
  }
}

fun getPathFromContentDataColumn (
  activity: MainActivity,
  uri: android.net.Uri,
  id: String? = null
) : String? {
  val context = activity.applicationContext
  val column = android.provider.MediaStore.MediaColumns.DATA
  var cursor: android.database.Cursor? = null
  var result: String? = null

  try {
    cursor = context.contentResolver.query(uri, arrayOf(column), null, null, null)
    if (cursor != null) {
      cursor.moveToFirst()
      do {
        if (id == null) {
          result = cursor.getString(cursor.getColumnIndex(column))
          break
        } else {
          var index = cursor.getColumnIndex(android.provider.MediaStore.MediaColumns._ID)
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
    return null
  } finally {
    if (cursor != null) {
      cursor.close()
    }
  }

  return result
}

fun isDocumentUri (activity: MainActivity, uri: android.net.Uri): Boolean {
  return android.provider.DocumentsContract.isDocumentUri(activity.applicationContext, uri)
}

fun isContentUri (uri: android.net.Uri): Boolean {
  return uri.scheme == "content"
}

fun isSocketUri (uri: android.net.Uri): Boolean {
  return uri.scheme == "socket"
}

fun isAssetBundleUri (uri: android.net.Uri): Boolean {
  val path = uri.path

  if (path == null) {
    return false
  }

  return (
    (isContentUri(uri) || isSocketUri(uri)) &&
    uri.authority == "__BUNDLE_IDENTIFIER__"
  )
}

fun isExternalStorageDocumentUri (uri: android.net.Uri): Boolean {
  return uri.authority == "com.android.externalstorage.documents"
}

fun isDownloadsDocumentUri (uri: android.net.Uri): Boolean {
  return uri.authority == "com.android.providers.downloads.documents"
}

fun isMediaDocumentUri (uri: android.net.Uri): Boolean {
  return uri.authority == "com.android.providers.media.documents"
}

fun getPathFromURI (activity: MainActivity, uri: android.net.Uri) : String? {
  // just return the file path for `file://` based URIs
  if (uri.scheme == "file") {
    return uri.path
  }

  if (isDocumentUri(activity, uri)) {
    if (isExternalStorageDocumentUri(uri)) {
      val externalStorage = android.os.Environment.getExternalStorageDirectory().absolutePath
      val documentId = android.provider.DocumentsContract.getDocumentId(uri)
      val parts = documentId.split(":")
      val type = parts[0]

      if (type == "primary") {
        return externalStorage + "/" + parts[1]
      }
    }

    if (isDownloadsDocumentUri(uri)) {
      val documentId = android.provider.DocumentsContract.getDocumentId(uri)
      val contentUri = android.content.ContentUris.withAppendedId(
        android.net.Uri.parse("content://downloads/public_downloads"),
        documentId.toLong()
      )

      return getPathFromContentDataColumn(activity, contentUri)
    }

    if (isMediaDocumentUri(uri)) {
      val documentId = android.provider.DocumentsContract.getDocumentId(uri)
      val parts = documentId.split(":")
      val type = parts[0]
      val id = parts[1]

      val contentUri = (
        if (type == "image") {
          android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI
        } else if (type == "video") {
          android.provider.MediaStore.Video.Media.EXTERNAL_CONTENT_URI
        } else if (type == "audio") {
          android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
        } else {
          android.provider.MediaStore.Files.getContentUri("external")
        }
      )

      if (contentUri == null) {
        return null
      }

      return getPathFromContentDataColumn(activity, contentUri, id)
    }
  } else if (uri.scheme == "content") {
    if (uri.authority == "com.google.android.apps.photos.content") {
      return uri.lastPathSegment
    }

    return getPathFromContentDataColumn(activity, uri)
  }

  return null
}

open class Bridge (runtime: Runtime, configuration: IBridgeConfiguration) {
  open protected val TAG = "Bridge"
  var pointer = alloc(runtime.pointer)
  var runtime = java.lang.ref.WeakReference(runtime)
  val requests = mutableMapOf<Long, RouteRequest>()
  val configuration = configuration
  val buffers = mutableMapOf<String, ByteArray>()
  val semaphore = java.util.concurrent.Semaphore(
    java.lang.Runtime.getRuntime().availableProcessors()
  )

  val openedAssetDirectories = mutableMapOf<String, String>()
  val openedAssetDirectoriesEntryCache: MutableMap<String, Array<String>> = mutableMapOf()
  val fileDescriptors = mutableMapOf<String, android.content.res.AssetFileDescriptor>()
  val uris = mutableMapOf<String, android.net.Uri>()

  protected var nextRequestId = 0L

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  fun call (command: String, callback: RouteCallback? = null): Boolean {
    return this.call(command, emptyMap(), callback)
  }

  fun call (command: String, options: Map<String, String> = emptyMap(), callback: RouteCallback? = null): Boolean {
    val message = Message("ipc://$command")

    for (entry in options.entries.iterator()) {
      message.set(entry.key, entry.value)
    }

    if (callback != null) {
      return this.route(message.toString(), null, callback)
    }

    return this.route(message.toString(), null, {})
  }

  fun route (
    value: String,
    bytes: ByteArray? = null,
    callback: RouteCallback
  ): Boolean {
    val activity = this.runtime.get()?.activity?.get() ?: return false
    val runtime = activity.runtime
    val message = Message(value)
    val contentResolver = activity.applicationContext.contentResolver
    val assetManager = activity.applicationContext.resources.assets

    message.bytes = bytes

    if (buffers.contains(message.seq)) {
      message.bytes = buffers[message.seq]
      buffers.remove(message.seq)
    }

    when (message.command) {
      "application.getScreenSize",
      "application.getWindows" -> {
        val windowManager = activity.applicationContext.getSystemService(
          android.content.Context.WINDOW_SERVICE
        ) as android.view.WindowManager

        val metrics = windowManager.getCurrentWindowMetrics()
        val windowInsets = metrics.windowInsets
        val insets = windowInsets.getInsetsIgnoringVisibility(
          android.view.WindowInsets.Type.navigationBars() or
          android.view.WindowInsets.Type.displayCutout()
        )

        val width = insets.right + insets.left
        val height = insets.top + insets.bottom

        if (message.command == "application.getScreenSize") {
          callback(Result(0, message.seq, message.command, """{
            "data": {
              "width": $width,
              "height": $height
            }
          }"""))
        } else {
          activity.runOnUiThread {
            val status = 31 // WINDOW_SHOWN"
            val title = activity.webview?.title ?: ""
            callback(Result(0, message.seq, message.command, """{
              "data": [{
                "index": 0,
                "title": "$title",
                "width": $width,
                "height": $height,
                "status": $status
              }]
            }"""))
          }
        }
        return true
      }

      "window.showFileSystemPicker" -> {
        val options = WebViewFilePickerOptions(
          null,
          arrayOf<String>(),
          message.get("allowMultiple") == "true",
          message.get("allowFiles") == "true",
          message.get("allowDirs") == "true"
        )

        activity.showFileSystemPicker(options, fun (uris: Array<android.net.Uri>) {
          var paths: Array<String> = arrayOf()

          for (uri in uris) {
            val path = getPathFromURI(activity, uri) ?: uri
            paths += "\"$path\""
          }

          callback(Result(0, message.seq, message.command, """{
            "data": {
              "paths": ${paths.joinToString(prefix = "[", postfix = "]")}
            }
          }"""))
        })
        return true
      }

      "os.paths" -> {
        val storage = android.os.Environment.getExternalStorageDirectory().absolutePath

        val resources = "socket://__BUNDLE_IDENTIFIER__"
        var downloads = "$storage/Downloads"
        var documents = "$storage/Documents"
        var pictures = "$storage/Pictures"
        var desktop = activity.getExternalFilesDir(null)?.absolutePath ?: "$storage/Desktop"
        var videos = "$storage/DCIM/Camera/"
        var music = "$storage/Music"
        var home = desktop

        callback(Result(0, message.seq, message.command, """{
          "data": {
            "resources": "$resources",
            "downloads": "$downloads",
            "documents": "$documents",
            "pictures": "$pictures",
            "desktop": "$desktop",
            "videos": "$videos",
            "music": "$music",
            "home": "$home"
          }
        }"""))
        return true
      }

      "permissions.request" -> {
        if (!message.has("name")) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "Expecting 'name' in parameters" }
          }"""))
          return true
        }

        val name = message.get("name")
        val permissions = mutableListOf<String>()

        when (name) {
          "geolocation" -> {
            if (
              activity.checkPermission("android.permission.ACCESS_COARSE_LOCATION") &&
              activity.checkPermission("android.permission.ACCESS_FINE_LOCATION")
            ) {
              callback(Result(0, message.seq, message.command, "{}"))
              return true
            }

            permissions.add("android.permission.ACCESS_COARSE_LOCATION")
            permissions.add("android.permission.ACCESS_FINE_LOCATION")
          }

          "push", "notifications" -> {
            if (activity.checkPermission("android.permission.POST_NOTIFICATIONS")) {
              callback(Result(0, message.seq, message.command, "{}"))
              return true
            }

            permissions.add("android.permission.POST_NOTIFICATIONS")
          }

          else -> {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "Unknown permission requested: '$name'"
              }
            }"""))
            return true
          }
        }

        activity.requestPermissions(permissions.toTypedArray(), fun (granted: Boolean) {
          if (granted) {
            callback(Result(0, message.seq, message.command, "{}"))
          } else {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "User denied permission request for '$name'"
              }
            }"""))
          }
        })

        return true
      }

      "permissions.query" -> {
        if (!message.has("name")) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "Expecting 'name' in parameters" }
          }"""))
          return true
        }

        val name = message.get("name")

        if (name == "geolocation") {
          if (!runtime.isPermissionAllowed("geolocation")) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "User denied permissions to access the device's location"
              }
            }"""))
          } else if (
            activity.checkPermission("android.permission.ACCESS_COARSE_LOCATION") &&
            activity.checkPermission("android.permission.ACCESS_FINE_LOCATION")
          ) {
            callback(Result(0, message.seq, message.command, """{
              "data": {
                "state": "granted"
              }
            }"""))
          } else {
            callback(Result(0, message.seq, message.command, """{
              "data": {
                "state": "prompt"
              }
            }"""))
          }
        }

        if (name == "notifications" || name == "push") {
          if (!runtime.isPermissionAllowed("notifications")) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "User denied permissions to show notifications"
              }
            }"""))
          } else if (
            activity.checkPermission("android.permission.POST_NOTIFICATIONS") &&
            androidx.core.app.NotificationManagerCompat.from(activity).areNotificationsEnabled()
          ) {
            callback(Result(0, message.seq, message.command, """{
              "data": {
                "state": "granted"
              }
            }"""))
          } else {
            callback(Result(0, message.seq, message.command, """{
              "data": {
                "state": "prompt"
              }
            }"""))
          }
        }

        if (name == "persistent-storage" || name == "storage-access") {
          if (!runtime.isPermissionAllowed("data_access")) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "User denied permissions for ${name.replace('-', ' ')}"
              }
            }"""))
          }
        }

        return true
      }

      "notification.show" -> {
        if (
          !activity.checkPermission("android.permission.POST_NOTIFICATIONS") ||
          !androidx.core.app.NotificationManagerCompat.from(activity).areNotificationsEnabled()
        ) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "User denied permissions for 'notifications'" }
          }"""))
          return true
        }

        if (!message.has("id")) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "Expecting 'id' in parameters" }
          }"""))
          return true
        }

        if (!message.has("title")) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "Expecting 'title' in parameters" }
          }"""))
          return true
        }

        val id = message.get("id")
        val channel = message.get("channel", "default").replace("default", "__BUNDLE_IDENTIFIER__");
        val vibrate = message.get("vibrate")
          .split(",")
          .filter({ it.length > 0 })
          .map({ it.toInt().toLong() })
          .toTypedArray()

        val identifier = id.toLongOrNull()?.toInt() ?: (0..16384).random().toInt()

        val contentIntent = android.content.Intent(activity, MainActivity::class.java).apply {
          flags = (
            android.content.Intent.FLAG_ACTIVITY_SINGLE_TOP or
            android.content.Intent.FLAG_ACTIVITY_CLEAR_TOP
          )
        }

        val deleteIntent = android.content.Intent(activity, MainActivity::class.java).apply {
          flags = (
            android.content.Intent.FLAG_ACTIVITY_SINGLE_TOP or
            android.content.Intent.FLAG_ACTIVITY_CLEAR_TOP
          )
        }

        contentIntent.setAction("notification.response.default")
        contentIntent.putExtra("id", id)

        deleteIntent.setAction("notification.response.dismiss")
        deleteIntent.putExtra("id", id)

        val pendingContentIntent: android.app.PendingIntent = android.app.PendingIntent.getActivity(
          activity,
          identifier,
          contentIntent,
          (
            android.app.PendingIntent.FLAG_UPDATE_CURRENT or
            android.app.PendingIntent.FLAG_IMMUTABLE or
            android.app.PendingIntent.FLAG_ONE_SHOT
          )
        )

        val pendingDeleteIntent: android.app.PendingIntent = android.app.PendingIntent.getActivity(
          activity,
          identifier,
          deleteIntent,
          (
            android.app.PendingIntent.FLAG_UPDATE_CURRENT or
            android.app.PendingIntent.FLAG_IMMUTABLE
          )
        )

        val builder = androidx.core.app.NotificationCompat.Builder(
          activity,
          channel
        )

        builder
          .setPriority(androidx.core.app.NotificationCompat.PRIORITY_DEFAULT)
          .setContentTitle(message.get("title", "Notification"))
          .setContentIntent(pendingContentIntent)
          .setDeleteIntent(pendingDeleteIntent)
          .setAutoCancel(true)

        if (message.has("body")) {
          builder.setContentText(message.get("body"))
        }

        if (message.has("icon")) {
          val url = message.get("icon")
            .replace("socket://__BUNDLE_IDENTIFIER__", "https://appassets.androidplatform.net/assets")
            .replace("https://__BUNDLE_IDENTIFIER__", "https://appassets.androidplatform.net/assets")

          val icon = androidx.core.graphics.drawable.IconCompat.createWithContentUri(url)
          builder.setSmallIcon(icon)
        } else {
          val icon = androidx.core.graphics.drawable.IconCompat.createWithResource(
            activity,
            R.mipmap.ic_launcher_round
          )
          builder.setSmallIcon(icon)
        }

        if (message.has("image")) {
          val url = message.get("image")
            .replace("socket://__BUNDLE_IDENTIFIER__", "https://appassets.androidplatform.net/assets")
            .replace("https://__BUNDLE_IDENTIFIER__", "https://appassets.androidplatform.net/assets")

          val icon = android.graphics.drawable.Icon.createWithContentUri(url)
          builder.setLargeIcon(icon)
        }

        if (message.has("category")) {
          var category = message.get("category")
            .replace("msg", "message")
            .replace("-", "_")

          builder.setCategory(category)
        }

        if (message.get("silent") == "true") {
          builder.setSilent(true)
        }

        val notification = builder.build()
        with (androidx.core.app.NotificationManagerCompat.from(activity)) {
          notify(
            message.get("tag"),
            identifier,
            notification
          )
        }

        callback(Result(0, message.seq, message.command, """{
          "data": {
            "id": "$id"
          }
        }"""))

        activity.runOnUiThread {
          this.emit("notificationpresented", """{
            "id": "$id"
          }""")
        }

        return true
      }

      "notification.close" -> {
        if (
          !activity.checkPermission("android.permission.POST_NOTIFICATIONS") ||
          !androidx.core.app.NotificationManagerCompat.from(activity).areNotificationsEnabled()
        ) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "User denied permissions for 'notifications'" }
          }"""))
          return true
        }

        if (!message.has("id")) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "Expecting 'id' in parameters" }
          }"""))
          return true
        }

        val id = message.get("id")
        with (androidx.core.app.NotificationManagerCompat.from(activity)) {
          cancel(
            message.get("tag"),
            id.toLongOrNull()?.toInt() ?: (0..16384).random().toInt()
          )
        }

        callback(Result(0, message.seq, message.command, """{
          "data": {
            "id": "$id"
          }
        }"""))

        activity.runOnUiThread {
          this.emit("notificationresponse", """{
            "id": "$id",
            "action": "dismiss"
          }""")
        }

        return true
      }

      "notification.list" -> {
        if (
          !activity.checkPermission("android.permission.POST_NOTIFICATIONS") ||
          !androidx.core.app.NotificationManagerCompat.from(activity).areNotificationsEnabled()
        ) {
          callback(Result(0, message.seq, message.command, """{
            "err": { "message": "User denied permissions for 'notifications'" }
          }"""))
          return true
        }

        return true
      }

      "buffer.map" -> {
        if (bytes != null) {
          buffers[message.seq] = bytes
        }
        callback(Result(0, message.seq, message.command, "{}"))
        return true
      }

      "log", "stdout" -> {
        console.log(message.value)
        callback(Result(0, message.seq, message.command, "{}"))
        return true
      }

      "stderr" -> {
        console.error(message.value)
        callback(Result(0, message.seq, message.command, "{}"))
        return true
      }

      "application.exit", "process.exit", "exit" -> {
        val code = message.get("value", "0").toInt()
        this.runtime.get()?.exit(code)
        callback(Result(0, message.seq, message.command, "{}"))
        return true
      }

      "openExternal" -> {
        this.runtime.get()?.openExternal(message.value)
        callback(Result(0, message.seq, message.command, "{}"))
        return true
      }

      "fs.access" -> {
        var mode = message.get("mode", "0").toInt()
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)
        if (isAssetBundleUri(uri) || isContentUri(uri) || isDocumentUri(activity, uri)) {
          try {
            val path = uri.path?.substring(1)
            val stream = (
              if (path != null && isAssetBundleUri(uri)) assetManager.open(path, mode)
              else contentResolver.openInputStream(uri)
            )

            if (stream == null) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "message": "Failed to open input stream for access check"
                }
              }"""))
              return true;
            }

            mode = 4 // R_OK
            stream.close()
            callback(Result(0, message.seq, message.command, """{
              "data": {
                "mode": $mode
              }
            }"""))
          } catch (e: java.io.FileNotFoundException) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "type": "NotFoundError",
                "message": "${e.message}"
              }
            }"""))
          } catch (e: Exception) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "${e.message}"
              }
            }"""))
          }

          return true
        }
      }

      "fs.open" -> {
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)

        if (isAssetBundleUri(uri) || isContentUri(uri) || isDocumentUri(activity, uri)) {
          val id = message.get("id")

          if (id.length == 0) {
            return false
          }

          try {
            val path = uri.path?.substring(1)
            val fd = (
              if (path != null && isAssetBundleUri(uri)) assetManager.openFd(path)
              else contentResolver.openTypedAssetFileDescriptor(
                uri,
                "*/*",
                null
              )
            )

            if (fd == null) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "message": "Failed to open asset file descriptor"
                }
              }"""))
              return true;
            }

            this.fileDescriptors[id] = fd
            this.uris[id] = uri

            callback(Result(0, message.seq, message.command, """{
              "data": {
                "id": "$id",
                "fd": ${fd.getParcelFileDescriptor().getFd()}
              }
            }"""))
          } catch (e: java.io.FileNotFoundException) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "type": "NotFoundError",
                "message": "${e.message}"
              }
            }"""))
          } catch (e: Exception) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "${e.message}"
              }
            }"""))
          }

          return true
        }
      }

      "fs.opendir" -> {
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)
        val id = message.get("id")

        if (isAssetBundleUri(uri)) {
          var path = uri.path
          if (path == null) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "Missing pathspec in 'path'"
              }
            }"""))
          } else {
            if (path.length > 0) {
              while (path != null && path.startsWith("/")) {
                path = path.substring(1)
              }

              if (path == null) {
                path = ""
              }

              if (path.length > 0 && !path.endsWith("/")) {
                path += "/"
              }
            }

            val entries = assetManager.list(path)
            if (entries == null || entries.size == 0) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "type": "NotFoundError",
                  "message": "Directory not found in asset manager"
                }
              }"""))
            } else {
              this.openedAssetDirectoriesEntryCache[id] = entries
              this.openedAssetDirectories[id] = path
              callback(Result(0, message.seq, message.command, """{
                "data": {
                  "id": "$id"
                }
              }"""))
            }
          }

          return true
        }
      }

      "fs.close" -> {
        val id = message.get("id")
        if (this.fileDescriptors.contains(id)) {
          try {
            val fd = this.fileDescriptors[id]
            this.fileDescriptors.remove(id)
            this.uris.remove(id)

            if (fd != null) {
              fd.close()
            }

            callback(Result(0, message.seq, message.command, """{
              "data": {
                "id": "$id"
              }
            }"""))
          } catch (e: Exception) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "${e.message}"
              }
            }"""))
          }

          return true
        }
      }

      "fs.closedir" -> {
        val id = message.get("id")
        if (this.openedAssetDirectories.contains(id)) {
          this.openedAssetDirectories.remove(id)
          this.openedAssetDirectoriesEntryCache.remove(id)
          callback(Result(0, message.seq, message.command, """{
            "data": {
              "id": "$id"
            }
          }"""))
          return true
        }
      }

      "fs.read" -> {
        val id = message.get("id")
        val size = message.get("size", "0").toInt()
        val offset = message.get("offset", "0").toLong()
        if (this.fileDescriptors.contains(id)) {
          try {
            val uri = this.uris[id]

            if (uri == null) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "message": "Could not determine URI for open asset file descriptor"
                }
              }"""))
              return true
            }

            val path = uri.path?.substring(1)
            val stream = (
              if (path != null && isAssetBundleUri(uri)) assetManager.open(path, 2)
              else contentResolver.openInputStream(uri)
            )

            if (stream == null) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "message": "Failed to open input stream for file read"
                }
              }"""))
              return true;
            }

            val bytes = ByteArray(size)

            if (offset > 0) {
              stream.skip(offset)
            }

            val bytesRead = stream.read(bytes, 0, size)
            stream.close()
            if (bytesRead > 0) {
              callback(Result(0, message.seq, message.command, "{}", bytes.slice(0..(bytesRead - 1)).toByteArray()))
            } else {
              callback(Result(0, message.seq, message.command, "{}", ByteArray(0)))
            }
          } catch (e: Exception) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "${e.message}"
              }
            }"""))
          }

          return true
        }
      }

      "fs.readdir" -> {
        val id = message.get("id")
        val max = message.get("entries", "8").toInt()
        if (this.openedAssetDirectories.contains(id)) {
          val path = this.openedAssetDirectories[id]
          if (path != null) {
            var entries = this.openedAssetDirectoriesEntryCache[id]
            if (entries != null) {
              var data: Array<String> = arrayOf()
              var count = 0
              for (entry in entries) {
                var type = 1

                try {
                  val tmp = assetManager.list(path + entry)
                  if (entry.endsWith("/") || (tmp != null && tmp.size > 0)) {
                    type = 2
                  }
                } catch (e: Exception) {}

                data += """{ "name": "$entry", "type": $type }"""

                if (++count == max) {
                  break
                }
              }

              entries = entries.slice(count..(entries.size - 1)).toTypedArray()
              this.openedAssetDirectoriesEntryCache[id] = entries

              callback(Result(0, message.seq, message.command, """{
                "data": ${data.joinToString(prefix = "[", postfix = "]")}
              }"""))
              return true;
            }
          }
          callback(Result(0, message.seq, message.command, """{ "data": [] }"""))
          return true;
        }
      }

      "fs.readFile" -> {
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)
        if (isAssetBundleUri(uri) || isContentUri(uri) || isDocumentUri(activity, uri)) {
          try {
            val path = uri.path?.substring(1)
            val stream = (
              if (path != null && isAssetBundleUri(uri)) assetManager.open(path, 2) // ACCESS_STREAMING
              else contentResolver.openInputStream(uri)
            )

            if (stream == null) {
              callback(Result(0, message.seq, message.command, """{
                "err": {
                  "message": "Failed to open input stream for file read"
                }
              }"""))
              return true;
            }

            val bytes = stream.readAllBytes()
            stream.close()
            callback(Result(0, message.seq, message.command, "{}", bytes))
          } catch (e: java.io.FileNotFoundException) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "type": "NotFoundError",
                "message": "${e.message}"
              }
            }"""))
          } catch (e: Exception) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "${e.message}"
              }
            }"""))
          }
          return true
        }
      }

      "fs.stat" -> {
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)
        if (isAssetBundleUri(uri) || isContentUri(uri) || isDocumentUri(activity, uri)) {
          val path = uri.path?.substring(1)
          val fd = (
            if (path != null && isAssetBundleUri(uri)) assetManager.openFd(path)
            else contentResolver.openTypedAssetFileDescriptor(
              uri,
              "*/*",
              null
            )
          )

          if (fd == null) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "Failed to open asset file descriptor for stats"
              }
            }"""))
            return true
          }

          callback(Result(0, message.seq, message.command, """{
            "data": {
              "st_mode": 4,
              "st_size": ${fd.getParcelFileDescriptor().getStatSize()}
            }
          }"""))

          fd.close()
          return true
        }
      }

      "fs.fstat" -> {
        val path = message.get("path")
        val id = message.get("id")
        if (this.fileDescriptors.contains(id)) {
          val fd = this.fileDescriptors[id]

          if (fd == null) {
            callback(Result(0, message.seq, message.command, """{
              "err": {
                "message": "Failed to acquire open asset file descriptor for stats"
              }
            }"""))
            return true
          }

          callback(Result(0, message.seq, message.command, """{
            "data": {
              "st_mode": 4,
              "st_size": ${fd.getParcelFileDescriptor().getStatSize()}
            }
          }"""))
          return true
        }
      }
    }

    if (message.domain == "fs") {
      if (message.has("id") || message.has("path")) {
        val path = message.get("path")
        val uri = android.net.Uri.parse(path)
        val id = message.get("id")
        if (isAssetBundleUri(uri) || isContentUri(uri) || isDocumentUri(activity, uri) || this.fileDescriptors.contains(id)) {
          callback(Result(0, message.seq, message.command, """{
            "err": {
              "type": "NotFoundError",
              "message": "'${message.command}' is not supported for Android content URIs"
            }
          }"""))
          return true
        }
      }

      val root = java.nio.file.Paths.get(configuration.getRootDirectory())
      if (message.has("path")) {
        var path = message.get("path")
        message.set("path", root.resolve(java.nio.file.Paths.get(path)).toString())
      }

      if (message.has("src")) {
        var src = message.get("src")
        message.set("src", root.resolve(java.nio.file.Paths.get(src)).toString())
      }

      if (message.has("dest")) {
        var dest = message.get("dest")
        message.set("dest", root.resolve(java.nio.file.Paths.get(dest)).toString())
      }

      if (message.has("dst")) {
        var dest = message.get("dst")
        message.set("dst", root.resolve(java.nio.file.Paths.get(dest)).toString())
      }
    }

    val request = RouteRequest(this.nextRequestId++, callback)
    this.requests[request.id] = request

    return this.route(message.toString(), message.bytes, request.id)
  }

  fun onInternalRouteResponse (
    id: Long,
    seq: String,
    source: String,
    value: String? = null,
    headersString: String? = null,
    bytes: ByteArray? = null
  ) {
    val headers = try {
      headersString
        ?.split("\n")
        ?.map { it.split(":", limit=3) }
        ?.map { it.elementAt(0) to it.elementAt(1) }
        ?.toMap()
      } catch (err: Exception) {
        null
      }

    val result = Result(id, seq, source, value ?: "", bytes, headers ?: emptyMap<String, String>())

    this.onResult(result)
  }

  open fun onResult (result: Result) {
    val semaphore = this.semaphore
    val activity = this.runtime.get()?.activity?.get()

    this.requests[result.id]?.apply {
      kotlin.concurrent.thread {
        semaphore.acquireUninterruptibly()

        if (activity != null) {
          activity.runOnUiThread {
            semaphore.release()
          }
        }

        callback(result)

        if (activity == null) {
          semaphore.release()
        }
      }
    }

    if (this.requests.contains(result.id)) {
      this.requests.remove(result.id)
    }
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (runtimePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun route (msg: String, bytes: ByteArray?, requestId: Long): Boolean;

  @Throws(java.lang.Exception::class)
  external fun emit (event: String, data: String = ""): Boolean;
}
