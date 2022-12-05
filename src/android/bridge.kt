// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IBridgeConfiguration {
  val evaluateJavascript: (String) -> Unit
}

data class BridgeConfiguration (
  override val evaluateJavascript: (String) -> Unit
) : IBridgeConfiguration

data class Result (
  val id: Long,
  val seq: String,
  val source: String,
  val value: String,
  val bytes: ByteArray,
  val headers: Map<String, String>
)

typealias RouteCallback = (Result) -> Unit

data class RouteRequest (
  val id: Long,
  val callback: RouteCallback
)

open class Bridge (runtime: Runtime, configuration: IBridgeConfiguration) {
  open protected val TAG = "Bridge"
  public var pointer = alloc(runtime.pointer)
  public var runtime = WeakReference(runtime)
  public val requests = mutableMapOf<Long, RouteRequest>()
  public val configuration = configuration

  protected var nextRequestId = 0L

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  fun route (
    msg: String,
    bytes: ByteArray?,
    callback: RouteCallback
  ): Boolean {
    val id = this.nextRequestId++
    val request = RouteRequest(id, callback)
    this.requests[id] = request
    return this.route(msg, bytes, id)
  }

  fun onInternalRouteResponse (
    id: Long,
    seq: String,
    source: String,
    value: String,
    headersString: String,
    bytes: ByteArray
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

    android.util.Log.d(TAG, "onInternalRouteResponse: $id $seq $source $value")
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
