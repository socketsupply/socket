// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IRuntimeConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
  val exit: (Int) -> Unit
  val openExternal: (String) -> Unit
}

data class RuntimeConfiguration (
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager,
  override val exit: (Int) -> Unit,
  override val openExternal: (String) -> Unit
) : IRuntimeConfiguration

data class RuntimeServiceWorkerFetchResponse (
  val id: Long,
  val statusCode: Int,
  val headers: Map<String, String> = emptyMap(),
  val bytes: ByteArray? = null
)

typealias RuntimeServiceWorkerFetchRequestCallback = (RuntimeServiceWorkerFetchResponse) -> Unit

data class RuntimeServiceWorkerFetchRequest (
  val id: Long,
  val callback: RuntimeServiceWorkerFetchRequestCallback
)

open class RuntimeServiceWorkerContainer (
  runtime: Runtime
) {
  val runtime = runtime
  val requests = mutableMapOf<Long, RuntimeServiceWorkerFetchRequest>()
  protected var nextFetchRequestId = 0L

  fun fetch (
    request: android.webkit.WebResourceRequest
  ): android.webkit.WebResourceResponse? {
    val url = request.url
    val method = request.method
    val headers = request.requestHeaders ?: mutableMapOf<String, String>()

    val path = url.path ?: return null
    val query = url.query ?: ""

    return this.fetch(method, path, query, headers)
  }

  fun fetch (
    method: String,
    path: String,
    query: String,
    requestHeaders: MutableMap<String, String>,
    bytes: ByteArray? = null
  ): android.webkit.WebResourceResponse? {
    val activity = runtime.activity.get() ?: return null
    var pathname = path
    var headers = ""

    if (pathname.startsWith("/assets/")) {
      pathname = pathname.replace("/assets/", "/")
    }

    for (entry in requestHeaders.iterator()) {
      headers += "${entry.key}: ${entry.value}\n"
    }

    val requestId = this.nextFetchRequestId++

    val stream = java.io.PipedOutputStream()
    val response = android.webkit.WebResourceResponse(
      "text/html",
      "utf-8",
      java.io.PipedInputStream(stream)
    )

    val fetchRequest = RuntimeServiceWorkerFetchRequest(requestId, fun (res: RuntimeServiceWorkerFetchResponse) {
      var contentType = ""

      response.apply {
        if (res.statusCode == 0) {
          setStatusCodeAndReasonPhrase(502, "Network Error")
          return stream.close()
        }

        when (res.statusCode) {
          200 -> { setStatusCodeAndReasonPhrase(res.statusCode, "OK") }
          201 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Created") }
          202 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Accepted") }
          204 -> { setStatusCodeAndReasonPhrase(res.statusCode, "No Content") }
          205 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Reset Content") }
          206 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Partial Content") }
          300 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Multiple Choices") }
          301 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Moved Permanently") }
          302 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Found") }
          304 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Not Modified") }
          307 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Temporary Redirect") }
          308 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Permanent Redirect") }
          400 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Bad Request") }
          401 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Unauthorized") }
          402 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Payment Required") }
          403 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Forbidden") }
          404 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Not Found") }
          405 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Method Not Allowed") }
          406 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Not Acceptable") }
          408 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Request Timeout") }
          409 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Conflict") }
          410 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Gone") }
          411 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Length Required") }
          412 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Precondition Failed") }
          413 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Payload Too Large") }
          414 -> { setStatusCodeAndReasonPhrase(res.statusCode, "URI Too Long") }
          415 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Unsupported Media Type") }
          416 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Range Not Satisfiable") }
          417 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Expectation Failed") }
          421 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Misdirected Request") }
          422 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Unprocessable Content") }
          423 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Locked") }
          425 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Too Early") }
          426 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Upgrade Required") }
          428 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Precondition Required") }
          429 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Too Many Requests") }
          431 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Request Header Fields Too Large") }
          500 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Internal Server Error") }
          501 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Not Implemented") }
          502 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Bad Gateway") }
          503 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Service Unavailable") }
          504 -> { setStatusCodeAndReasonPhrase(res.statusCode, "Gateway Timeout") }
          else -> {
            setStatusCodeAndReasonPhrase(res.statusCode, "Unknown Status")
          }
        }

        setResponseHeaders(res.headers)

        for (entry in res.headers.iterator()) {
          if (entry.key.lowercase() == "content-type") {
            contentType = entry.value
            break
          }
        }

        if (contentType.length > 0) {
          setMimeType(contentType)
        }
      }

      stream.apply {
        var bytes = res.bytes
        if (bytes != null) {
          if (contentType == "text/html" || pathname.endsWith(".html")) {
            val preload = (
              """
              <meta name="runtime-frame-source" content="serviceworker" />
              ${activity.window.getJavaScriptPreloadSource()}
              """
            )

            var html = String(bytes)

            if (html.contains("<head>")) {
              html = html.replace("<head>", """
              <head>
              $preload
              """)
            } else if (html.contains("<body>")) {
              html = html.replace("<body>", """
              $preload
              <body>
              """)
            } else if (html.contains("<html>")){
              html = html.replace("<html>", """
              <html>
              $preload
              """)
            } else {
              html = preload + html
            }

            bytes = html.toByteArray()
          }

          try {
            write(bytes, 0, bytes.size)
          } catch (err: Exception) {
            if (err.message != "Pipe closed") {
              console.error("RuntimeServiceWorkerContainer.fetch(): ${err.toString()}")
            }
          }
        }

        kotlin.concurrent.thread {
          close()
        }
      }
    })

    val fetched = runtime.serviceWorkerContainerFetch(
      requestId,
      pathname,
      method,
      query,
      headers,
      bytes
    )

    if (!fetched) {
      stream.close()
      return null
    }

    this.requests[requestId] = fetchRequest

    return response
  }
}

open class Runtime (
  activity: MainActivity,
  configuration: RuntimeConfiguration
) {
  var pointer = alloc(activity.getRootDirectory())
  var activity = WeakReference(activity)
  val configuration = configuration;
  val serviceWorker = RuntimeServiceWorkerContainer(this)
  val semaphore = java.util.concurrent.Semaphore(
    java.lang.Runtime.getRuntime().availableProcessors()
  )

  var isRunning = false

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  fun exit (code: Int) {
    this.configuration.exit(code)
  }

  fun openExternal (value: String) {
    this.configuration.openExternal(value)
  }

  fun start () {
    if (!this.isRunning) {
      this.resume()
      this.isRunning = true
    }
  }

  fun stop () {
    if (this.isRunning) {
      this.pause()
      this.isRunning = false
    }
  }

  fun destroy () {
    this.stop()
    this.finalize()
  }

  fun onInternalServiceWorkerFetchResponse (
    id: Long,
    statusCode: Int,
    headersString: String? = null,
    bytes: ByteArray? = null
  ) {
    val activity = this.activity.get()
    val headers = try {
      headersString
        ?.split("\n")
        ?.map { it.split(":", limit=2) }
        ?.map { it.elementAt(0) to it.elementAt(1) }
        ?.toMap()
      } catch (err: Exception) {
        null
      }

    val response = RuntimeServiceWorkerFetchResponse(
      id,
      statusCode,
      headers ?: emptyMap<String, String>(),
      bytes
    )

    this.serviceWorker.requests[id]?.apply {
      kotlin.concurrent.thread {
        callback(response)
      }
    }

    if (this.serviceWorker.requests.contains(id)) {
      this.serviceWorker.requests.remove(id)
    }
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (rootDirectory: String): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  external fun isDebugEnabled (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun pause (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun resume (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun startEventLoop (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun stopEventLoop (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun startTimers (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun stopTimers (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun isPermissionAllowed (permission: String): Boolean;

  @Throws(java.lang.Exception::class)
  external fun setIsEmulator (value: Boolean): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getConfigValue (key: String): String;

  @Throws(java.lang.Exception::class)
  external fun serviceWorkerContainerFetch (
    requestId: Long,
    pathname: String,
    method: String,
    query: String,
    headers: String
  ): Boolean
}
