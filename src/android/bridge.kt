// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

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
  var runtime = WeakReference(runtime)
  val requests = mutableMapOf<Long, RouteRequest>()
  val configuration = configuration
  val buffers = mutableMapOf<String, ByteArray>()

  protected var nextRequestId = 0L

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  fun route (
    value: String,
    bytes: ByteArray? = null,
    callback: RouteCallback
  ): Boolean {
    val message = Message(value)
    message.bytes = bytes

    if (buffers.contains(message.seq)) {
      message.bytes = buffers[message.seq]
      buffers.remove(message.seq)
    }

    when (message.command) {
      "buffer.map" -> {
        if (bytes != null) {
          buffers[message.seq] = bytes
        }
        return true
      }

      "log", "stdout" -> {
        console.log(message.value)
        return true
      }

      "stderr" -> {
        console.error(message.value)
        return true
      }

      "exit" -> {
        val code = message.get("value", "0").toInt()
        this.runtime.get()?.exit(code)
        return true
      }

      "openExternal" -> {
        this.runtime.get()?.openExternal(message.value)
        return true
      }

      "process.cwd" -> {
        val json = """{
          "source": "process.cwd",
          "data": "${configuration.getRootDirectory()}"
        }""".trim()
        callback(Result(9, message.seq, message.command, json))
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
    }

    val request = RouteRequest(this.nextRequestId++, callback)
    this.requests[request.id] = request

    return this.route(message.toString(), message.bytes, request.id)
  }

  fun onInternalRouteResponse (
    id: Long,
    seq: String,
    source: String,
    value: String,
    headersString: String,
    bytes: ByteArray? = null
  ) {
    val headers = try {
      headersString
        .split("\n")
        .map { it.split(":", limit=2) }
        .map { it.elementAt(0) to it.elementAt(1) }
        .toMap()
      } catch (err: Exception) {
        emptyMap<String, String>()
      }

    this.onResult(Result(id, seq, source, value, bytes, headers))
  }

  open fun onResult (result: Result) {
    this.requests[result.id]?.apply {
      callback(result)
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
}
