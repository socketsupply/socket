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

    message.bytes = bytes

    if (buffers.contains(message.seq)) {
      message.bytes = buffers[message.seq]
      buffers.remove(message.seq)
    }

    when (message.command) {
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
    }

    if (message.domain == "fs") {
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
        ?.map { it.split(":", limit=2) }
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
