// vim: set sw=2:
package __BUNDLE_IDENTIFIER__.old

fun decodeURIComponent(string: String): String {
  val normalized = string.replace("+", "%2B")
  return java.net.URLDecoder.decode(normalized, "UTF-8").replace("%2B", "+")
}

fun isAssetUri (uri: android.net.Uri): Boolean {
  if (
      uri.scheme == "file" &&
      uri.host == "" &&
      uri.pathSegments.get(0) == "android_asset"
  ) {
    return true
  }

  if (uri.host == "appassets.androidplatform.net") {
    return true
  }

  return false
}

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView(context: android.content.Context) : android.webkit.WebView(context)

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient(activity: WebViewActivity) : android.webkit.WebViewClient() {
  protected val activity = activity
  open protected val TAG = "WebViewClient"
  open protected var assetLoader: androidx.webkit.WebViewAssetLoader? = null

  init {
    this.assetLoader = androidx.webkit.WebViewAssetLoader.Builder()
      .addPathHandler(
        "/assets/",
        androidx.webkit.WebViewAssetLoader.AssetsPathHandler(activity)
      )
      .build()
  }

  /**
   * Handles URL loading overrides for "file://" based URI schemes.
   */
  override fun shouldOverrideUrlLoading(
    view: android.webkit.WebView, request: android.webkit.WebResourceRequest
  ): Boolean {
    val url = request.url

    if (url.scheme == "ipc") {
      return true
    }

    if (url.scheme == "file") {
      return true
    }

    if (isAssetUri(url)) {
      return true
    }

    val intent = android.content.Intent(android.content.Intent.ACTION_VIEW, url)

    try {
      this.activity.startActivity(intent)
    } catch (err: Error) {
      // @TODO(jwelre): handle this error gracefully
      android.util.Log.e(TAG, err.toString())
      return false
    }

    return true
  }

  override fun shouldInterceptRequest(
    view: android.webkit.WebView,
    request: android.webkit.WebResourceRequest
  ): android.webkit.WebResourceResponse? {
    val url = request.url
    val assetLoaderRequest = this.assetLoader?.shouldInterceptRequest(url)

    // android.util.Log.d(TAG, "${url.scheme} ${request.method}")

    if (assetLoaderRequest != null) {
      return assetLoaderRequest
    }

    if (url.scheme != "ipc") {
      return null
    }

    when (url.host) {
      "ping" -> {
        val stream = java.io.ByteArrayInputStream("pong".toByteArray())
        val response = android.webkit.WebResourceResponse(
          "text/plain",
          "utf-8",
          stream
        )

        response.responseHeaders = mapOf(
          "Access-Control-Allow-Origin" to "*"
        )

        return response
      }

      "post" -> {
        if (request.method == "GET") {
          val core = this.activity.core
          val id = url.getQueryParameter("id")

          if (core == null || id == null) {
            return null
          }

          val bytes = core.getPostData(id)
          val stream = java.io.ByteArrayInputStream(bytes)
          val response = android.webkit.WebResourceResponse(
            "application/octet-stream",
            "utf-8",
            stream
          )

          response.responseHeaders = mapOf(
            "Access-Control-Allow-Origin" to "*",
            "Access-Control-Allow-Headers" to "*"
          )

          core.freePostData(id)

          return response
        }
      }

      else -> {
        val bridge = this.activity.bridge ?: return null

        if (request.method == "OPTIONS") {
          val stream = java.io.PipedOutputStream()
          val response = android.webkit.WebResourceResponse(
            "text/plain",
            "utf-8",
            java.io.PipedInputStream(stream)
          )

          response.responseHeaders = mapOf(
            "Access-Control-Allow-Origin" to "*",
            "Access-Control-Allow-Headers" to "*",
            "Access-Control-Allow-Methods" to "*"
          )

          stream.close()
          return response
        }

        val core = this.activity.core
        val value = url.toString()
        val stream = java.io.PipedOutputStream()
        val message = IPCMessage(value)
        val response = android.webkit.WebResourceResponse(
          "application/octet-stream",
          "utf-8",
          java.io.PipedInputStream(stream)
        )

        response.responseHeaders = mapOf(
          "Access-Control-Allow-Origin" to "*",
          "Access-Control-Allow-Headers" to "*",
          "Access-Control-Allow-Methods" to "*"
        )

        // try interfaces
        for ((name, _) in bridge.interfaces) {
          val returnValue = bridge.invokeInterface(
            name,
            message,
            value,
            callback = fun (_: String, data: String, postId: String) {
              try {
                if (postId != "" && postId != "0") {
                  core?.apply {
                    val bytes = getPostData(postId)
                    android.util.Log.d(TAG, "${url.host} ${bytes.size} bytes for ${postId}")
                    stream.write(bytes, 0, bytes.size)
                    freePostData(postId)
                  }
                } else {
                  val bytes = data.toByteArray()
                  stream.write(bytes, 0, bytes.size)
                }

                stream.close()
              } catch (err: Exception) {
                android.util.Log.d(TAG, "ERR (${url.host}): $err")
              }
            },

            throwError = { seq: String, error: String ->
              stream.write(error.toByteArray(), 0, error.length)
              stream.close()
              seq
            }
          )

          if (returnValue != null) {
            return response
          }
        }

        val err = JSONError(
          id = message.get("id"),
          type = "NotFoundError",
          extra = "\"url\": \"$url\"",
          message = "Not found",
          source = url.host
        ).toString()

        android.util.Log.d(TAG, "ERR: $err")

        response.setStatusCodeAndReasonPhrase(404, "Not found")
        stream.write(err.toByteArray(), 0, err.length)
        stream.close()

        return response
      }
    }

    return null
  }

  override fun onPageStarted(
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    android.util.Log.d(TAG, "WebViewClient is loading: $url")

    val core = activity.core

    if (core != null && core.isReady) {
      val source = core.getJavaScriptPreloadSource()
      view.evaluateJavascript(source, null)
    } else {
      android.util.Log.e(TAG, "NativeCore is not ready in WebViewClient")
    }
  }
}

/**
 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 * @TODO(jwerle): look into `androidx.appcompat.app.AppCompatActivity`
 */
open class WebViewActivity : androidx.appcompat.app.AppCompatActivity() {
  open protected val TAG = "WebViewActivity"
  open protected var client: WebViewClient? = null
  open protected val timer = java.util.Timer()

  open var externalInterface: ExternalWebViewInterface? = null
  open var bridge: Bridge? = null
  open var view: android.webkit.WebView? = null
  open var core: NativeCore? = null

  /**
   * Called when the `WebViewActivity` is first created
   * @see https://developer.android.com/reference/kotlin/android/app/Activity#onCreate(android.os.Bundle)
   */
  override fun onCreate(state: android.os.Bundle?) {
    super.onCreate(state)

    setContentView(R.layout.web_view_activity)

    val client = WebViewClient(this)
    val externalInterface = ExternalWebViewInterface(this)

    this.externalInterface = externalInterface
    this.bridge = Bridge(this)
    this.client = client
    this.view = findViewById(R.id.webview)
    this.core = NativeCore(this).apply {
      configure(
        GenericNativeCoreConfiguration(
          rootDirectory = getExternalFilesDir(null)?.absolutePath ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files",
          assetManager = applicationContext.resources.assets
        )
      )

      if (check()) {
        val index = getPathToIndexHTML()

        android.webkit.WebView.setWebContentsDebuggingEnabled(isDebugEnabled)

        view?.apply {
          settings.javaScriptEnabled = true

          // allow list
          settings.allowFileAccess = true
          settings.allowContentAccess = true
          settings.allowFileAccessFromFileURLs = true

          webViewClient = client
          addJavascriptInterface(externalInterface, "external")
          loadUrl("https://appassets.androidplatform.net/assets/$index")
        }
      }
    }

    val core = this.core
    this.timer.schedule(
      kotlin.concurrent.timerTask {
        core?.apply {
          android.util.Log.d(TAG, "Expiring old post data")
          expirePostData()
        }
      },
      30L * 1024L, // delay
      30L * 1024L //period
    )
  }

  override fun onDestroy() {
    this.timer?.apply {
      cancel()
      purge()
    }

    this.core?.apply {
      freeAllPostData()
      stopEventLoop()
    }

    super.onDestroy()
  }

  override fun onPause () {
    this.core?.apply {
      stopEventLoop()
      pauseAllPeers()
    }

    super.onPause()
  }

  override fun onResume () {
    this.core?.apply {
      runEventLoop()
      resumeAllPeers()
    }

    super.onResume()
  }
}

/**
 * @TODO
 */
open class Bridge(activity: WebViewActivity) {

  /**
   * A reference to the core `WebViewActivity`
   */
  protected val activity = activity

  val buffers = mutableMapOf<String, ByteArray>()

  /**
   * Registered invocation interfaces.
   */
  val interfaces = mutableMapOf<
    String, // name
    ( // callback
      IPCMessage,
      String,
      (String, String, String) -> Unit,
      (String, String) -> String
    ) -> String?
  >()

  companion object {
    const val TAG = "Bridge"
    const val OK_STATE = "0"
    const val ERROR_STATE = "1"
  }

  fun evaluateJavascript(
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    this.activity.externalInterface?.evaluateJavascript(source, callback)
  }

  /**
   * @TODO
   */
  fun send(
    seq: String,
    message: String?,
    state: String = Bridge.OK_STATE
  ): Boolean {
    val source = activity.core?.getResolveToRenderProcessJavaScript(
      seq,
      state,
      message ?: "",
      true
    )

    if (source != null) {
      this.evaluateJavascript("queueMicrotask(() => { $source; });")
      return true
    }

    return false
  }

  /**
   * @TODO
   */
  fun throwError(seq: String, message: String?): String {
    this.send(seq, "\"$message\"", Bridge.ERROR_STATE)
    return seq
  }

  /**
   * Registers a bridge interface by name and callback
   */
  fun registerInterface(
    name: String,
    callback: (
      IPCMessage,
      String,
      (String, String, String) -> Unit,
      (String, String) -> String
    ) -> String?,
  ) {
    interfaces[name] = callback
  }

  fun invokeInterface(
    name: String,
    message: IPCMessage,
    value: String,
    callback: (String, String, String) -> Unit,
    throwError: (String, String) -> String
  ): String? {
    if (this.buffers[message.seq] != null && message.bytes == null) {
      message.bytes = this.buffers[message.seq]
      this.buffers.remove(message.seq)
    }

    return interfaces[name]?.invoke(message, value, callback, throwError)
  }

  /**
   * `ExternalWebViewInterface` class initializer
   */
  init {
    this.registerInterface("bluetooth", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "bluetooth.start", "bluetooth-start" -> {
          // @TODO
          return message.seq
        }

        "bluetooth.subscribe", "bluetooth-subscribe" -> {
          // @TODO
          return message.seq
        }

        "bluetooth.publish", "bluetooth-publish" -> {
          // @TODO
          return message.seq
        }
      }

      return null
    })

    /*
    this.registerInterface("buffer", fun (
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core
      val bytes = message.bytes

      if (core == null) {
        return null
      }

      if (message.seq.isEmpty()) {
        return null
      }

      when (message.command) {
        "buffer.map" -> {
          if (bytes != null) {
            this.buffers[message.seq] = bytes
          }

          return message.seq
        }

        "buffer.release" -> {
          this.buffers.remove(message.seq)
          return message.seq
        }
      }

      return null
    })
    */

    /*
    this.registerInterface("dns", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "dnsLookup", "dns.lookup" -> {
          val hostname = message.get("hostname")
          val family = message.get("family", "4").toInt()
          val seq = message.seq

          if (hostname.isEmpty()) {
            throw RuntimeException("dns.lookup: Missing 'hostname' in IPC")
          }

          core.dns.lookup(seq, hostname, family, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }
      }

      return null
    })

    /*
    this.registerInterface("fs", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "fs.access", "fsAccess" -> {
          if (!message.has("path")) {
            return throwError(message.seq, "'path' is required")
          }

          var path = message.get("path")
          val uri = android.net.Uri.parse(path)

          if (isAssetUri(uri)) {
            val pathSegments = uri.getPathSegments()
            path = pathSegments
              .slice(1..pathSegments.size - 1)
              .joinToString("/")

            // @TODO(jwerle): core.fs.accessAsset()
          } else {
            val seq = message.seq
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val mode = message.get("mode", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.access(seq, resolved.toString(), mode, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.chmod", "fsChmod" -> {
          if (!message.has("path")) {
            return throwError(message.seq, "'path' is required")
          }

          if (!message.has("mode")) {
            return throwError(message.seq, "'path' is required")
          }

          var path = message.get("path")
          val uri = android.net.Uri.parse(path)

          if (isAssetUri(uri)) {
            val pathSegments = uri.getPathSegments()
            path = pathSegments
              .slice(1..pathSegments.size - 1)
              .joinToString("/")

            // @TODO(jwerle): core.fs.chmodAsset()
          } else {
            val seq = message.seq
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val mode = message.get("mode").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.chmod(seq, resolved.toString(), mode, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.close", "fsClose" -> {
          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          val id = message.get("id")

          if (core.fs.isAssetId(id)) {
            core.fs.closeAsset(id, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          } else {
            core.fs.close(message.seq, id, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.constants", "fsConstants", "getFSConstants" -> {
          val constants = core.getFSConstants()
          callback(message.seq, constants, "")
          return message.seq
        }

        "fs.fstat", "fsFStat" -> {
          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          val id = message.get("id")

          if (core.fs.isAssetId(id)) {
            // @TODO fstatAsset
          } else {
            core.fs.fstat(message.seq, id, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.open", "fsOpen" -> {
          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          if (!message.has("path")) {
            return throwError(message.seq, "'path' is required")
          }

          var path = message.get("path")
          val uri = android.net.Uri.parse(path)
          val id = message.get("id")

          if (isAssetUri(uri)) {
            val pathSegments = uri.pathSegments
            path = pathSegments
              .slice(1 until pathSegments.size)
              .joinToString("/")

            core.fs.openAsset(id, path, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          } else {
            val seq = message.seq
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val mode = message.get("mode", "0").toInt()
            val flags = message.get("flags", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.open(seq, id, resolved.toString(), flags, mode, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.read", "fsRead" -> {
          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          if (!message.has("size")) {
            return throwError(message.seq, "'size' is required")
          }

          val id = message.get("id")
          val size = message.get("size").toInt()
          val offset = message.get("offset", "0").toInt()

          if (core.fs.isAssetId(id)) {
            core.fs.readAsset(message.seq, id, offset, size, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          } else {
            core.fs.read(message.seq, id, offset, size, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.stat", "fsStat" -> {
          var path = message.get("path")
          val uri = android.net.Uri.parse(path)

          if (isAssetUri(uri)) {
            val pathSegments = uri.pathSegments
            path = pathSegments
              .slice(1 until pathSegments.size)
              .joinToString("/")

            // @TODO: statAsset()
          } else {
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val flags = message.get("flags", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.stat(message.seq, resolved.toString(), fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }

        "fs.write", "fsWrite" -> {
          val bytes = message.bytes

          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          if (bytes == null) {
            return throwError(message.seq, "Missing required bytes")
          }

          val id = message.get("id")
          val offset = message.get("offset", "0").toInt()

          if (core.fs.isAssetId(id)) {
            callback(message.seq, JSONError(id, "AssetManager does not support writes").toString(), "")
          } else {
            core.fs.write(message.seq, id, bytes, offset, fun(data: String, postId: String) {
              callback(message.seq, data, postId)
            })
          }

          return message.seq
        }
      }

      return null
    })
    */

    /*
    this.registerInterface("bufferSize", fun (
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      if (message.seq.isEmpty()) {
        return null
      }

      when (message.command) {
        "bufferSize" -> {
          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          val id = message.get("id")
          val size = message.get("size", "0").toInt()
          val buffer = message.get("buffer", "0").toInt()

          core.bufferSize(message.seq, id, size, buffer, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })
          return message.seq
        }
      }

      return null
    })
    */

    /*
    this.registerInterface("os", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "os.networkInterfaces", "getNetworkInterfaces" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("getNetworkInterfaces: Missing 'seq' in IPC")
          }

          callback(message.seq, core.getNetworkInterfaces(), "")
          return message.seq
        }

        "os.arch", "getPlatformArch" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("getPlatformArch: Missing 'seq' in IPC")
          }

          callback(message.seq, core.getPlatformArch(), "")
          return message.seq
        }

        "os.type", "getPlatformType" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("getPlatformType: Missing 'seq' in IPC")
          }

          callback(message.seq, core.getPlatformType(), "")
          return message.seq
        }

        "os.platform", "getPlatformOS" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("getPlatformOS: Missing 'seq' in IPC")
          }

          callback(message.seq, core.getPlatformOS(), "")
          return message.seq
        }
      }

      return null
    })
    */

    /*
    this.registerInterface("process", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "process.cwd", "cwd" -> {
          callback(message.seq, core.getRootDirectory() ?: "", "")
          return message.seq
        }
      }

      return null
    })
    */

    /*
    this.registerInterface("udp", fun(
      message: IPCMessage,
      value: String,
      callback: (String, String, String) -> Unit,
      throwError: (String, String) -> String
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "udpClose", "udp.close" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.close: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.close(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpBind", "udp.bind" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.bind: Missing 'seq' in IPC")
          }

          val id = message.get("id")
          val port = message.get("port", "0").toInt()
          val address = message.get("address")
          val reuseAddr = message.get("reuseAddr", "false") == "true"

          core.udp.bind(message.seq, id, address, port, reuseAddr, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpConnect", "udp.connect" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.connect: Missing 'seq' in IPC")
          }

          val id = message.get("id")
          val port = message.get("port", "0").toInt()
          val address = message.get("address")

          core.udp.connect(message.seq, id, address, port, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpDisdisconnect", "udp.disconnect" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.disconnect: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.disconnect(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpGetPeerName", "udp.getPeerName" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.getPeerName: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.getPeerName(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpGetSockName", "udp.getSockName" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.getSockName: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.getSockName(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpGetState", "udp.getState" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.getState: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.getState(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpReadStart", "udp.readStart" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.readStart: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.readStart(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpReadStop", "udp.readStop" -> {
          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.readStop: Missing 'seq' in IPC")
          }

          val id = message.get("id")

          core.udp.readStop(message.seq, id, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }

        "udpSend", "udp.send" -> {
          val bytes = message.bytes

          if (message.seq.isEmpty()) {
            throw RuntimeException("udp.send: Missing 'seq' in IPC")
          }

          if (!message.has("id")) {
            return throwError(message.seq, "'id' is required")
          }

          if (!message.has("address")) {
            return throwError(message.seq, "'address' is required")
          }

          if (bytes == null) {
            return throwError(message.seq, "Missing required bytes")
          }

          val id = message.get("id")
          val port = message.get("port", "0").toInt()
          val size = message.get("size", bytes.size.toString()).toInt()
          val address = message.get("address")
          val ephemeral = message.get("ephemeral") == "true"

          // android.util.Log.d(TAG, "message= ${message}")

          core.udp.send(message.seq, id, bytes, size, address, port, ephemeral, fun (data: String, postId: String) {
            callback(message.seq, data, postId)
          })

          return message.seq
        }
      }

      return null
    })
    */
  }
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class ExternalWebViewInterface(activity: WebViewActivity) {
  val TAG = "ExternalWebViewInterface"

  /**
   * A reference to the core `WebViewActivity`
   */
  val activity = activity


  fun evaluateJavascript(
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    activity.runOnUiThread {
      activity.view?.evaluateJavascript(source, callback)
    }
  }

  fun throwGlobalError(message: String) {
    val source = "throw new Error(\"$message\")"
    evaluateJavascript(source, null)
  }

  @android.webkit.JavascriptInterface
  final fun invoke(value: String): String? {
    return this.invoke(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  final fun invoke(value: String, bytes: ByteArray?): String? {
    val core = this.activity.core
    val bridge = this.activity.bridge
    val message = IPCMessage(value)

    message.bytes = bytes

    if (core == null || !core.isReady) {
      throwGlobalError("Missing NativeCore in WebViewActivity")
      return null
    }

    if (bridge == null) {
      throw RuntimeException("Missing Bridge in WebViewActivity")
    }

    if (message.command.isEmpty()) {
      throw RuntimeException("Invoke: Missing 'command' in IPC")
    }

    when (message.command) {
      "log", "stdout" -> {
        android.util.Log.d(TAG, message.value)
        return null
      }

      "title" -> {
        // noop
        return null
      }

      "exit" -> {
        android.util.Log.d(TAG, "__EXIT_SIGNAL__=${message.value}")
        this.activity.finishAndRemoveTask()
        return message.seq
      }

      "window.eval", "eval" -> {
        try {
          this.evaluateJavascript(decodeURIComponent(message.get("value")), fun (result: String?) {
            bridge.send(message.seq, result)
          })
        } catch (err: Exception) {
          return bridge.throwError(message.seq, JSONError("null", err.toString()).toString())
        }

        return message.seq
      }

      "external", "openExternal" -> {
        require(message.value.isNotEmpty()) { "openExternal: Missing 'value' (URL) in IPC" }

        this.activity.startActivity(
          android.content.Intent(
            android.content.Intent.ACTION_VIEW, android.net.Uri.parse(message.value)
          )
        )

        return null
      }
    }

    // try interfaces
    for ((name, _) in bridge.interfaces) {
      val returnValue =
        bridge.invokeInterface(
          name,
          message,
          value,
          callback = { seq: String, data: String, postId: String ->
            if (postId != "" && postId != "0") {
              val bytes = core.getPostData(postId)
              core?.apply {
                evaluateJavascript(createPost(seq, postId, data, "", bytes))
              }
            } else if (seq.isNotEmpty() || data.isNotEmpty()) {
              bridge.send(seq, data)
            }
          },

          throwError = { seq: String, error: String ->
            bridge.throwError(seq, error)
          })

      if (returnValue != null) {
        return returnValue
      }
    }

    val err = JSONError(
      id = message.get("id"),
      type = "NotFoundError",
      message = "Not found",
      source = message.command
    ).toString()

    if (message.has("seq")) {
      bridge.send(message.seq, err)
    }

    return err
  }
}

/**
 * A container for a parseable IPC message (ipc://...)
 */
class IPCMessage(message: String?) {

  /**
   * Internal URI representation of an `ipc://...` message
   */
  var uri: android.net.Uri? =
    if (message != null) {
      android.net.Uri.parse(message)
    } else {
      android.net.Uri.parse("ipc://")
    }

  /**
   * `command` in IPC URI accessor.
   */
  var command: String
    get() = uri?.host ?: ""
    set(command) {
      uri = uri?.buildUpon()?.authority(command)?.build()
    }

  /**
   * `value` in IPC URI query accessor.
   */
  var value: String
    get() = get("value")
    set(value) {
      set("value", value)
    }

  /**
   * `seq` in IPC URI query accessor.
   */
  var seq: String
    get() = get("seq")
    set(seq) {
      set("seq", seq)
    }

  /**
   * `state` in IPC URI query accessor.
   */
  var state: String
    get() = get("state", Bridge.OK_STATE)
    set(state) {
      set("state", state)
    }

  var bytes: ByteArray? = null

  fun get(key: String, defaultValue: String = ""): String {
    val value = uri?.getQueryParameter(key)

    return if (value != null && value.isNotEmpty()) {
      value
    } else {
      defaultValue
    }
  }

  fun has(key: String): Boolean {
    return get(key).isNotEmpty()
  }

  fun set(key: String, value: String): Boolean {
    uri = uri?.buildUpon()?.appendQueryParameter(key, value)?.build()

    return uri == null
  }

  fun delete(key: String): Boolean {
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
    if (uri == null) {
      return ""
    }

    return uri.toString()
  }
}

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.
 */
open class MainWebViewActivity : WebViewActivity()

class JSONError(
  private val id: String,
  private val message: String,
  private val extra: String = "",
  private val type: String = "InternalError",
  private val source: String? = "",
) {
  override fun toString() = """{
    "source": "${if (source != null) source else ""}",
    "err": {
      "id": "$id",
      "type": "$type",
      "message": "$message"${if (extra.isNotEmpty()) "," else ""}
      $extra
    }
  }"""
}

class JSONData(
  private val id: String,
  private val data: String = "",
  private val source: String = "",
) {
  override fun toString() = """{
    "source": "$source",
    "data": {
      "id": "$id" ${if (data.isNotEmpty()) "," else ""}
      $data
    }
  }"""
}

/**
 * Interface for `NativeCore` configuration.kj
 */
interface NativeCoreConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
}

/**
 * `NativeCore` class configuration used as input for `NativeCore::configure()`
 */
data class GenericNativeCoreConfiguration(
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager
) : NativeCoreConfiguration

/**
 * @TODO
 */
open class NativeDNS(core: NativeCore) {
  val TAG = "NativeDNS"

  private var core: NativeCore? = core

  fun lookup (
    seq: String = "",
    hostname: String,
    family: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      dnsLookup(seq, hostname, family, queueCallback(callback))
    }
  }

  fun finalize() {
    core = null
  }
}

open class NativeUDP(core: NativeCore) {
  val TAG = "NativeUDP"

  private var core: NativeCore? = core

  fun bind (
    seq: String = "",
    id: String,
    address: String,
    port: Int,
    reuseAddr: Boolean,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpBind(seq, id, address, port, reuseAddr, queueCallback(callback))
    }
  }

  fun close (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpClose(seq, id, queueCallback(callback))
    }
  }

  fun connect (
    seq: String = "",
    id: String,
    address: String,
    port: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpConnect(seq, id, address, port, queueCallback(callback))
    }
  }

  fun disconnect (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpDisconnect(seq, id, queueCallback(callback))
    }
  }

  fun getPeerName (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpGetPeerName(seq, id, queueCallback(callback))
    }
  }

  fun getSockName (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpGetSockName(seq, id, queueCallback(callback))
    }
  }

  fun getState (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpGetState(seq, id, queueCallback(callback))
    }
  }

  fun readStart (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpReadStart(seq, id, queueCallback(callback))
    }
  }

  fun readStop (
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpReadStop(seq, id, queueCallback(callback))
    }
  }

  fun send (
    seq: String = "",
    id: String,
    data: ByteArray,
    size: Int,
    address: String,
    port: Int,
    ephemeral: Boolean,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      udpSend(seq, id, data, size, address, port, ephemeral, queueCallback(callback))
    }
  }

  fun finalize() {
    core = null
  }
}


/**
 * @TODO
 */
open class NativeFileSystem(core: NativeCore) {
  val TAG = "NativeFileSystem"

  data class AssetDescriptor(
    val id: String,
    val path: String,
    var position: Int = 0,
    var fd: android.content.res.AssetFileDescriptor? = null,
    var stream: java.io.FileInputStream? = null
  );

  private var core: NativeCore? = core
  private val openAssets = mutableMapOf<String, AssetDescriptor>()

  var nextId: Long = 0
  val constants = mutableMapOf<String, Int>();

  init {
    val encodedConstants = core.getEncodedFSConstants().split('&')
    for (pair in encodedConstants) {
      val kv = pair.split('=')

      if (kv.size == 2) {
        val key = decodeURIComponent(kv[0])
        val value = decodeURIComponent(kv[1]).toInt()

        constants[key] = value
      }
    }
  }

  fun finalize() {
    core = null
  }

  fun getNextAvailableId(): String {
    return (++this.nextId).toString()
  }

  fun isAssetId(id: String): Boolean {
    if (openAssets[id] != null) {
      return true
    }

    return false
  }

  fun access(
    seq: String = "",
    path: String,
    mode: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsAccess(seq, path, mode, queueCallback(callback))
    }
  }

  fun chmod(
    seq: String = "",
    path: String,
    mode: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsChmod(seq, path, mode, queueCallback(callback))
    }
  }

  fun close(
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsClose(seq, id, queueCallback(callback))
    }
  }

  fun closeAsset(
    id: String,
    callback: (String, String) -> Unit
  ) {
    val descriptor = openAssets[id]
      ?: return callback(JSONError(id, "Invalid file descriptor").toString(), "")

    descriptor.fd?.close()
    descriptor.stream?.close()

    openAssets.remove(id)
    return callback(JSONData(id).toString(), "")
  }

  fun copyFile() {
  }

  fun fstat(
    seq: String = "",
    id: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      val callbackId = queueCallback(callback)
      fsFStat(seq, id, callbackId)
    }
  }

  fun mkdir() {
  }

  fun open(
    seq: String = "",
    id: String,
    path: String,
    flags: Int,
    mode: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsOpen(seq, id, path, flags, mode, queueCallback(callback))
    }
  }

  fun openAsset(
    id: String,
    path: String,
    callback: (String, String) -> Unit
  ) {
    val assetManager = core?.getAssetManager()
      ?: return callback(JSONError(id, "AssetManager is not initialized").toString(), "")

    val fd = try {
      assetManager.openFd(path)
    } catch (err: Exception) {
      var message = err.toString()

      when (err) {
        is java.io.FileNotFoundException -> {
          message = "No such file or directory: ${err.message}"
        }
      }

      callback(JSONError(id, message).toString(), "")

      null
    }

    if (fd == null) {
      return
    }

    if (!fd.fileDescriptor.valid()) {
      callback(JSONError(id, "Invalid file descriptor").toString(), "")
      return
    }

    fd.close()

    openAssets[id] = AssetDescriptor(id, path)

    callback(JSONData(id, "\"fd\": $id").toString(), "")
  }

  fun read(
    seq: String = "",
    id: String,
    offset: Int = 0,
    size: Int,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsRead(seq, id, size, offset, queueCallback(callback))
    }
  }

  fun readAsset(
    seq: String = "",
    id: String,
    offset: Int = 0,
    size: Int,
    callback: (String, String) -> Unit
  ) {
    val descriptor = openAssets[id]
      ?: return callback(JSONError(id, "Invalid file descriptor").toString(), "")

    val assetManager = core?.getAssetManager()
      ?: return callback(JSONError(id, "AssetManager is not initialized").toString(), "")

    val buffer = ByteArray(size.toInt())
    val start = (
      if (offset >= 0) kotlin.math.min(size, offset)
      else descriptor.position
    )

    if (descriptor.fd != null && offset >= 0 && offset < descriptor.position) {
      descriptor.fd?.close()
      descriptor.stream?.close()

      descriptor.fd = null
      descriptor.stream = null
    }

    if (descriptor.fd == null) {
      descriptor.fd = assetManager.openFd(descriptor.path)
      descriptor.stream = descriptor.fd?.createInputStream()
    }

    if (start > descriptor.position) {
      descriptor.position = descriptor.stream?.skip(
        start.toLong() - descriptor.position
      )?.toInt() ?: 0
    }

    val bytesRead = try {
      descriptor.stream?.read(buffer, 0, size)
    } catch (err: Exception) {
      return callback(JSONError(id, err.toString()).toString(), "")
    }

    if (bytesRead != null && bytesRead > 0) {
      val bytes = buffer.slice(0..bytesRead - 1).toByteArray()

      descriptor.position += bytesRead

      try {
        core?.apply {
          evaluateJavascript(createPost(seq, "0", "{}", "", bytes))
        }
      } catch (err: Exception) {
        return callback(JSONError(id, err.toString()).toString(), "")
      }
    }
  }

  fun readdir() {
  }

  fun rename() {
  }

  fun rmdir() {
  }

  fun stat(
    seq: String = "",
    path: String,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsStat(seq, path, queueCallback(callback))
    }
  }

  fun write(
    seq: String = "",
    id: String,
    data: ByteArray,
    offset: Int = 0,
    callback: (String, String) -> Unit
  ) {
    core?.apply {
      fsWrite(seq, id, data, offset, queueCallback(callback))
    }
  }

  fun unlink() {
  }
}

/**
 * NativeCore bindings externally implemented in JNI/NDK
 */
open class NativeCore(var activity: WebViewActivity) {
  val TAG = "NativeCore"

  /**
   * Internal pointer managed by `initialize() and `destroy()
   */
  var pointer: Long = 0

  /**
   * @TODO
   */
  var nextCallbackId: Long = 0

  /**
   * @TODO
   */
  var fs: NativeFileSystem

  /**
   * @TODO
   */
  var dns: NativeDNS

  /**
   * @TODO
   */
  var udp: NativeUDP

  /**
   * TODO
   */
  val callbacks = mutableMapOf<String, (String, String) -> Unit>()

  /**
   * Set internally by the native binding if debug is enabled.
   */
  var isDebugEnabled: Boolean = false

  /**
   * Set when all checks have passed.
   */
  var isReady: Boolean = false

  /**
   * Internal root directory for application passed directory
   * to JNI/SDK for `NativeCore`
   */
  private var rootDirectory: String? = null

  /**
   * Internal `AssetManager` instance passed directly to JNI/NDK
   * for the `NativeCore`
   */
  private var assetManager: android.content.res.AssetManager? = null

  /**
   * `NativeCore` singleton definitions
   */
  companion object {
    // Initialization for `NativeCore` singleton where we load the `ssc-core` library.
    init {
      println("Loading 'ssc-core' native library")
      System.loadLibrary("ssc-core")
    }
  }

  /**
   * `NativeCore` lifecycle functions
   **/
  external fun createPointer(): Long
  external fun destroyPointer(pointer: Long)

  /**
   * `NativeCore` vitals
   */
  @Throws(java.lang.Exception::class)
  external fun verifyFileSystem(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyLoop(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyRefs(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyJavaVM(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyPointer(pointer: Long): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyRootDirectory(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyAssetManager(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyNativeExceptions(): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyEnvironment(): Boolean

  /**
   * `NativeCore` internal utility bindings
   */
  @Throws(java.lang.Exception::class)
  external fun runEventLoop(): Boolean

  @Throws(java.lang.Exception::class)
  external fun stopEventLoop(): Boolean

  @Throws(java.lang.Exception::class)
  external fun resumeAllPeers(): Boolean

  @Throws(java.lang.Exception::class)
  external fun pauseAllPeers(): Boolean

  @Throws(java.lang.Exception::class)
  external fun configureEnvironment(): Boolean

  @Throws(java.lang.Exception::class)
  external fun configureWebViewWindow(): Boolean

  @Throws(java.lang.Exception::class)
  external fun getPathToIndexHTML(): String

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource(): String

  @Throws(java.lang.Exception::class)
  external fun getResolveToRenderProcessJavaScript(
    seq: String,
    state: String,
    value: String,
    shouldEncodeValue: Boolean
  ): String

  @Throws(java.lang.Exception::class)
  external fun getEmitToRenderProcessJavaScript(
    event: String,
    value: String
  ): String

  /**
   * `NativeCore` bindings
   */
  @Throws(java.lang.Exception::class)
  external fun getNetworkInterfaces(): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformArch(): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformType(): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformOS(): String

  @Throws(java.lang.Exception::class)
  external fun getEncodedFSConstants(): String

  @Throws(java.lang.Exception::class)
  external fun getFSConstants(): String

  @Throws(java.lang.Exception::class)
  external fun getPostData(id: String): ByteArray

  @Throws(java.lang.Exception::class)
  external fun freePostData(id: String)

  @Throws(java.lang.Exception::class)
  external fun freeAllPostData()

  @Throws(java.lang.Exception::class)
  external fun expirePostData()

  @Throws(java.lang.Exception::class)
  external fun createPost(
    seq: String,
    id: String,
    params: String,
    headers: String,
    bytes: ByteArray
  ): String

  @Throws(java.lang.Exception::class)
  external fun bufferSize(
    seq: String,
    id: String,
    size: Int,
    buffer: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  fun bufferSize(
    seq: String,
    id: String,
    size: Int,
    buffer: Int,
    callback: (String, String) -> Unit
  ) {
    bufferSize(seq, id, size, buffer, queueCallback(callback))
  }

  /**
   * DNS APIs
   */
  @Throws(java.lang.Exception::class)
  external fun dnsLookup(
    seq: String,
    hostname: String,
    family: Int,
    callback: String
  )

  /**
   * UDP APIs
   */
  @Throws(java.lang.Exception::class)
  external fun udpClose(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpBind(
    seq: String,
    id: String,
    ip: String,
    port: Int,
    reuseAddr: Boolean,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpConnect(
    seq: String,
    id: String,
    ip: String,
    port: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpDisconnect(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpGetPeerName(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpGetSockName(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpGetState(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpReadStart(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpReadStop(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun udpSend(
    seq: String,
    id: String,
    data: ByteArray,
    size: Int,
    ip: String,
    port: Int,
    ephemeral: Boolean,
    callback: String
  )

  /**
   * FileSystem APIs
   **/
  @Throws(java.lang.Exception::class)
  external fun fsAccess(
    seq: String,
    path: String,
    mode: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsChmod(
    seq: String,
    path: String,
    mode: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsClose(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsFStat(
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsOpen(
    seq: String,
    id: String,
    path: String,
    flags: Int,
    mode: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsRead(
    seq: String,
    id: String,
    size: Int,
    offset: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsStat(
    seq: String,
    path: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsWrite(
    seq: String,
    id: String,
    data: ByteArray,
    offset: Int,
    callback: String
  )

  init {
    this.pointer = this.createPointer()
    this.dns = NativeDNS(this)
    this.udp = NativeUDP(this)
    this.fs = NativeFileSystem(this)
  }

  /**
   * Kotlin class internal destructor where we call `destroy()`.
   */
  protected fun finalize() {
    android.util.Log.d(TAG, "Destroying internal NativeCore pointer")
    this.destroyPointer(this.pointer)

    this.assetManager = null
    this.pointer = 0
    this.fs.finalize()
  }

  /**
   * Configures `NativeCore` with Android dependencies.
   */
  @Throws(java.lang.Exception::class)
  fun configure(config: NativeCoreConfiguration): Boolean {
    if (isReady) {
      return true
    }

    this.rootDirectory = config.rootDirectory
    this.assetManager = config.assetManager

    if (!configureEnvironment()) {
      return false
    }

    if (!configureWebViewWindow()) {
      return false
    }

    // skip checks if not in debug mode
    if (isDebugEnabled == false) {
      isReady = true
    }

    return true
  }

  /**
   * Returns the internal `AssetManager` instance.
   * @notice This function is used by the `NativeCore`
   */
  fun getAssetManager(): android.content.res.AssetManager? {
    return this.assetManager
  }

  /**
   * Returns the internal rootDirectory string.
   * @notice This function is used by the `NativeCore`
   */
  fun getRootDirectory(): String? {
    return this.rootDirectory
  }

  /**
   * Returns the next available callback ID
   */
  fun getNextAvailableCallbackId(): String {
    return (this.nextCallbackId++).toString()
  }

  /**
   * @TODO
   */
  fun evaluateJavascript(source: String) {
    this.activity.externalInterface?.evaluateJavascript(source, null)
  }

  /**
   * @TODO
   */
  fun callback(id: String, data: String, postId: String) {
    this.callbacks[id]?.invoke(data, postId)
  }

  /**
   * @TODO
   */
  fun queueCallback(cb: (String, String) -> Unit): String {
    val id = this.getNextAvailableCallbackId()
    this.callbacks[id] = cb
    return id
  }

  /**
   * Performs internal vital checks.
   */
  fun check(): Boolean {
    if (isReady) {
      return true
    }

    if (!verifyPointer(pointer)) {
      android.util.Log.e(TAG, "pointer check failed")
      return false
    } else {
      android.util.Log.d(TAG, "pointer check OK")
    }

    // @TODO
    if (!verifyRefs()) {
      return false
    }
    if (!verifyJavaVM()) {
      return false
    }

    try {
      verifyNativeExceptions()
      android.util.Log.e(TAG, "native exceptions check failed")
      return false
    } catch (err: Exception) {
      if (err.message != "ExceptionCheck") {
        android.util.Log.e(TAG, "native exceptions check failed")
        return false
      }

      android.util.Log.d(TAG, "native exceptions check OK")
    }

    // @TODO
    if (!verifyEnvironment()) {
      return false
    }

    try {
      if (!verifyRootDirectory()) {
        android.util.Log.e(TAG, "root directory check failed")
        return false
      }
      android.util.Log.d(TAG, "root directory check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "root directory check failed")
    }

    try {
      if (!verifyAssetManager()) {
        android.util.Log.e(TAG, "asset manager check failed")
        return false
      }
      android.util.Log.d(TAG, "asset manager check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "asset manager check failed")
    }

    if (!verifyLoop()) {
      android.util.Log.e(TAG, "libuv loop check failed")
      return false
    } else {
      android.util.Log.d(TAG, "libuv loop check OK")
    }

    try {
      if (!verifyFileSystem()) {
        android.util.Log.e(TAG, "filesystem check failed")
        return false
      }

      android.util.Log.d(TAG, "filesystem check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "filesystem check failed")
      android.util.Log.e(TAG, err.toString())

      return false
    }

    isReady = true
    return true
  }
}
