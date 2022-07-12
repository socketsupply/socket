// vim: set sw=2:
package __BUNDLE_IDENTIFIER__

fun decodeURIComponent (string: String): String {
  val normalized = string.replace("+", "%2B")
  return java.net.URLDecoder.decode(normalized, "UTF-8").replace("%2B", "+")
}

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView(context: android.content.Context): android.webkit.WebView(context)

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient(activity: WebViewActivity) : android.webkit.WebViewClient() {
    protected val activity = activity
    open protected val TAG = "WebViewClient"

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

        if (url.scheme != "file") {
            return false
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
        view: android.webkit.WebView, request: android.webkit.WebResourceRequest
    ): android.webkit.WebResourceResponse? {
        val url = request.url

        android.util.Log.e(TAG, "${url.scheme} ${request.method}")

        if (url.scheme != "ipc") {
            return null
        }

        when (url.host) {
            "ping" -> {
                val stream = java.io.ByteArrayInputStream("pong".toByteArray())
                val response = android.webkit.WebResourceResponse(
                    "text/plain", "utf-8", stream
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

                    val stream = java.io.ByteArrayInputStream(core.getPostData(id))
                    val response = android.webkit.WebResourceResponse(
                        "application/octet-stream", "utf-8", stream
                    )

                    response.responseHeaders = mapOf(
                        "Access-Control-Allow-Origin" to "*"
                    )

                    core.freePostData(id)

                    return response
                }
            }

      else -> {
        val bridge = this.activity.bridge ?: return null

        val value = url.toString()
        val stream = java.io.PipedOutputStream()
        val message = IPCMessage(value)
        val response = android.webkit.WebResourceResponse(
          "text/plain",
          "utf-8",
          java.io.PipedInputStream(stream)
        )

        response.responseHeaders = mapOf(
            "Access-Control-Allow-Origin" to "*"
        )

                // try interfaces
                for ((name, _) in bridge.interfaces) {
                    val returnValue = bridge.invokeInterface(name,
                        message,
                        value,
                        callback = { _: String, data: String ->
                            android.util.Log.d(TAG, data)
                            stream.write(data.toByteArray(), 0, data.length)
                            stream.close()
                        },
                        throwError = { _: String, error: String ->
                            stream.write(error.toByteArray(), 0, error.length)
                            stream.close()
                        })

                    if (returnValue != null) {
                        return response
                    }
                }
            }
        }

        return null
    }

    override fun onPageStarted(
        view: android.webkit.WebView, url: String, bitmap: android.graphics.Bitmap?
    ) {
        android.util.Log.e(TAG, "WebViewClient is loading: $url")

        val core = activity.core

        if (core != null && core.isReady) {
            val source = core.getJavaScriptPreloadSource()
            view.evaluateJavascript(source) {
                android.util.Log.d(TAG, it)
            }
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
  open protected var client: WebViewClient? = null
  open protected val TAG = "WebViewActivity"

    open var externalInterface: ExternalWebViewInterface? = null
    open var bridge: Bridge? = null
    open var view: android.webkit.WebView? = null
    open var core: NativeCore? = null

    /**
     * Called when the `WebViewActivity` is starting
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
                    rootDirectory = getExternalFilesDir(null)?.absolutePath ?: "",
                    assetManager = applicationContext.resources.assets
                )
            )
            if (check()) {
                val index = getPathToIndexHTML()

                android.webkit.WebView.setWebContentsDebuggingEnabled(isDebugEnabled)

                view?.apply {
                    settings.javaScriptEnabled = true
                    webViewClient = client
                    addJavascriptInterface(externalInterface, "external")
                    loadUrl("file:///android_asset/$index")
                }
            }
        }
    }
}

/**
 * @TODO
 */
public open class Bridge(activity: WebViewActivity) {

  /**
   * A reference to the core `WebViewActivity`
   */
  protected val activity = activity

  /**
   * Registered invocation interfaces.
   */
  public val interfaces = mutableMapOf<
    String, // name
    ( // callback
      IPCMessage,
      String,
      (String, String) -> Unit,
      (String, String) -> Unit
    ) -> String?
  >()

  companion object {
    const val OK_STATE = "0"
    const val ERROR_STATE = "1"
    const val TAG = "Bridge"
  }

  fun evaluateJavascript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    this.activity.externalInterface?.evaluateJavascript(source, callback)
  }

  /**
   * @TODO
   */
  fun send (
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
            this.evaluateJavascript(source)
            return true
        }

        return false
    }

  /**
   * @TODO
   */
  public fun throwError (seq: String, message: String?) {
    this.send(seq, "\"$message\"", Bridge.ERROR_STATE)
  }

  /**
   * Registers a bridge interface by name and callback
   */
  public fun registerInterface (
    name: String,
    callback: (
      IPCMessage,
      String,
      (String, String) -> Unit,
      (String, String) -> Unit
    ) -> String?,
  ) {
    interfaces[name] = callback
  }

  public fun invokeInterface (
    name: String,
    message: IPCMessage,
    value: String,
    callback: (String, String) -> Unit,
    throwError: (String, String) -> Unit
  ): String? {
    return interfaces[name]?.invoke(message, value, callback, throwError)
  }

  /**
   * `ExternalWebViewInterface` class initializer
   */
  init {
    this.registerInterface("os", fun (
      message: IPCMessage,
      value: String,
      callback: (String, String) -> Unit,
      throwError: (String, String) -> Unit
    ): String? {
      val core = this.activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        "getNetworkInterfaces" -> {
          if (message.seq.length == 0) {
            throw RuntimeException("getNetworkInterfaces: Missing 'seq' in IPC.")
          }

          callback(message.seq, core.getNetworkInterfaces())
          return message.seq
        }

        "getPlatformArch" -> {
          if (message.seq.length == 0) {
            throw RuntimeException("getPlatformArch: Missing 'seq' in IPC.")
          }

          callback(message.seq, core.getPlatformArch())
          return message.seq
        }

        "getPlatformType" -> {
          if (message.seq.length == 0) {
            throw RuntimeException("getPlatformType: Missing 'seq' in IPC.")
          }

          callback(message.seq, core.getPlatformType())
          return message.seq
        }

        "getPlatformOS" -> {
          if (message.seq.length == 0) {
            throw RuntimeException("getPlatformOS: Missing 'seq' in IPC.")
          }

          callback(message.seq, core.getPlatformOS())
          return message.seq
        }
      }

      return null
    })

    this.registerInterface("fs", fun (
      message: IPCMessage,
      value: String,
      callback: (String, String) -> Unit,
      throwError: (String, String) -> Unit
    ): String? {
      val core = activity.core

      if (core == null) {
        return null
      }

      when (message.command) {
        // sync + async
        "fs.id", "fsId" -> {
          val id = core.fs.getNextAvailableId().toString()
          callback(message.seq, id)
          return id
        }

        "fs.constants", "fsConstants", "getFSConstants" -> {
          val constants = core.getFSConstants()
          callback(message.seq, constants)
          return message.seq
        }

        "fs.access", "fsAccess" -> {
          if (!message.has("path")) {
            throwError(message.seq, "'path' is required.")
            return null
          }

          var path = message.get("path")
          val uri = android.net.Uri.parse(path)

          if (
            uri.getScheme() == "file" &&
            uri.getHost() == "" &&
            uri.getPathSegments().get(0) == "android_asset"
          ) {
            val pathSegments = uri.getPathSegments()
            path = pathSegments
              .slice(1..pathSegments.size - 1)
              .joinToString("/")

              // @TODO
          } else {
            val seq = message.seq
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val mode = message.get("mode", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.access(seq, resolved.toString(), mode, fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }

        "fs.open", "fsOpen" -> {
          if (!message.has("id")) {
            throwError(message.seq, "'id' is required.")
            return null
          }

          if (!message.has("path")) {
            throwError(message.seq, "'path' is required.")
            return null
          }

          var path = message.get("path")
          val uri = android.net.Uri.parse(path)
          val id = message.get("id")

          if (
            uri.getScheme() == "file" &&
            uri.getHost() == "" &&
            uri.getPathSegments().get(0) == "android_asset"
          ) {
            val pathSegments = uri.getPathSegments()
            path = pathSegments
              .slice(1..pathSegments.size - 1)
              .joinToString("/")

            core.fs.openAsset(id, path, fun (data: String) {
              callback(message.seq, data)
            })
          } else {
            val seq = message.seq
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val mode = message.get("mode", "0").toInt()
            val flags = message.get("flags", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.open(seq, id, resolved.toString(), flags, mode, fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }

        "fs.close", "fsClose" -> {
          if (!message.has("id")) {
            throwError(message.seq, "'id' is required.")
            return null
          }

          val id = message.get("id")

          if (core.fs.isAssetId(id)) {
            core.fs.closeAsset(id, fun (data: String) {
              callback(message.seq, data)
            })
          } else {
            core.fs.close(message.seq, id, fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }

        "fs.read", "fsRead" -> {
          if (!message.has("id")) {
            throwError(message.seq, "'id' is required.")
            return null
          }

          if (!message.has("size")) {
            throwError(message.seq, "'size' is required.")
            return null
          }

          val id = message.get("id")
          val size = message.get("size").toInt()
          val offset = message.get("offset", "0").toInt()

          if (core.fs.isAssetId(id)) {
            core.fs.readAsset(id, offset, size, fun (data: String) {
              callback(message.seq, data)
            })
          } else {
            core.fs.read(message.seq, id, offset, size, fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }

        "fs.stat", "fsStat" -> {
          var path = message.get("path")
          val uri = android.net.Uri.parse(path)

          if (
            uri.getScheme() == "file" &&
            uri.getHost() == "" &&
            uri.getPathSegments().get(0) == "android_asset"
          ) {
            val pathSegments = uri.getPathSegments()
            path = pathSegments
              .slice(1..pathSegments.size - 1)
              .joinToString("/")

              // @TODO: statAsset()
          } else {
            val root = java.nio.file.Paths.get(core.getRootDirectory())
            val flags = message.get("flags", "0").toInt()
            val resolved = root.resolve(java.nio.file.Paths.get(path))

            core.fs.stat(message.seq, resolved.toString(), fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }

        "fs.fstat", "fsFStat" -> {
          if (!message.has("id")) {
            throwError(message.seq, "'id' is required.")
            return null
          }

          val id = message.get("id")

          if (core.fs.isAssetId(id)) {
            // @TODO fstatAsset
          } else {
            android.util.Log.d(TAG, "fstat($message.seq, $id)")
            core.fs.fstat(message.seq, id, fun (data: String) {
              callback(message.seq, data)
            })
          }

          return message.seq
        }
      }

      return null
    })
  }
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
public open class ExternalWebViewInterface(activity: WebViewActivity) {
  final protected val TAG = "ExternalWebViewInterface"

  /**
   * A reference to the core `WebViewActivity`
   */
  protected val activity = activity


  fun evaluateJavascript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    this.activity.runOnUiThread(fun () {
      android.util.Log.d(TAG, "evaluateJavascript: $source")
      this.activity.view?.evaluateJavascript(source, callback)
    })
  }

  fun throwGlobalError (message: String) {
    val source = "throw new Error(\"$message\")"
    this.evaluateJavascript(source, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  final fun invoke (value: String): String? {
    val core = this.activity.core
    val bridge = this.activity.bridge
    val message = IPCMessage(value)

    if (core == null || !core.isReady) {
      this.throwGlobalError("Missing NativeCore in WebViewActivity.")
      return null
    }

    if (bridge == null) {
      throw RuntimeException("Missing Bridge in WebViewActivity.")
    }

    if (message.command.length == 0) {
      throw RuntimeException("Invoke: Missing 'command' in IPC.")
    }

    when (message.command) {
      "log", "stdout" -> {
        android.util.Log.d(TAG, message.value)
        return null
      }

      "external", "openExternal" -> {
        if (message.value.length == 0) {
          throw RuntimeException("openExternal: Missing 'value' (URL) in IPC.")
        }

        this.activity.startActivity(
          android.content.Intent(
            android.content.Intent.ACTION_VIEW,
            android.net.Uri.parse(message.value)
          )
        )

        return null
      }
    }

    // try bridge interfaces
    for ((name, callback)  in bridge.interfaces) {
      val returnValue = bridge.invokeInterface(
        name,
        message,
        value,
        fun (seq: String, data: String) {
          if (seq.length  > 0|| data.length > 0) {
            bridge.send(seq, data)
          }
        },
        fun (seq: String, error: String) {
          bridge.throwError(seq, error)
        }
      )

      if (returnValue != null) {
        return returnValue
      }
    }

    if (message.has("seq")) {
      bridge.throwError(
        message.seq,
        "Unknown command in IPC: ${message.command}"
      )

      return null
    }

    throw RuntimeException("Invalid IPC invocation.")
  }
}

/**
 * A container for a parseable IPC message (ipc://...)
 */
class IPCMessage  {
  /**
   * Internal URI representation of an `ipc://...` message
   */
  internal var uri: android.net.Uri? = null

  /**
   * `command` in IPC URI accessor.
   */
  public var command: String
    get () = this.uri?.getHost() ?: ""
    set (command) {
      this.uri = this.uri
        ?.buildUpon()
        ?.authority(command)
        ?.build()
    }

  /**
   * `value` in IPC URI query accessor.
   */
  public var value: String
    get () = this.get("value")
    set (value) { this.set("value", value) }

  /**
   * `seq` in IPC URI query accessor.
   */
  public var seq: String
    get () = this.get("seq")
    set (seq) { this.set("seq", seq) }

  /**
   * `state` in IPC URI query accessor.
   */
  public var state: String
    get () = this.get("state", Bridge.OK_STATE)
    set (state) { this.set("state", state) }

  constructor (message: String? = null) {
    if (message != null) {
      this.uri = android.net.Uri.parse(message)
    } else {
      this.uri = android.net.Uri.parse("ipc://")
    }
  }

  public fun get (key: String, defaultValue: String = ""): String {
    val value = this.uri?.getQueryParameter(key)

    if (value != null && value.length > 0) {
      return value
    }

    return defaultValue
  }

  public fun has (key: String): Boolean {
    return this.get(key).length > 0
  }

  public fun set (key: String, value: String): Boolean {
    this.uri = this.uri
      ?.buildUpon()
      ?.appendQueryParameter(key, value)
      ?.build()

    if (this.uri == null) {
      return false
    }

    return true
  }

  public fun delete (key: String): Boolean {
    if (this.uri?.getQueryParameter(key) == null) {
      return false
    }

    val params = this.uri?.getQueryParameterNames()
    val tmp = this.uri?.buildUpon()?.clearQuery()

    if (params != null) {
      for (param: String in params) {
        if (!param.equals(key)) {
          val value = this.uri?.getQueryParameter(param)
          tmp?.appendQueryParameter(param, value)
        }
      }
    }

    this.uri = tmp?.build()

    return true
  }

  override public fun toString(): String {
    if (this.uri == null) {
      return ""
    }

    return this.uri.toString()
  }
}

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.
 */
public open class MainWebViewActivity : WebViewActivity()

public class JSONError(id: String, message: String, extra: String = "") {
  val id = id
  val extra = extra
  val message = message

  override fun toString () = """{
    "err": {
      "id": "$id",
      "message": "$message" ${if (extra.length > 0) "," else ""}
      $extra
    }
  }"""
}

public class JSONData(id: String, data: String = "") {
  val id = id
  val data = data

  override fun toString () = """{
    "data": {
      "id": "$id" ${if (data.length > 0) "," else ""}
      $data
    }
  }"""
}

/**
 * Interface for `NativeCore` configuration.kj
 */
public interface NativeCoreConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
}

/**
 * `NativeCore` class configuration used as input for `NativeCore::configure()`
 */
public data class GenericNativeCoreConfiguration(
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager
) : NativeCoreConfiguration

/**
 * @TODO
 */
public open class NativeFileSystem(core: NativeCore) {
  protected val TAG = "NativeFileSystem"

  val core = core
  var nextId: Long = 0

  val openAssets = mutableMapOf<String, String>()

  public val constants = mutableMapOf<String, Int>();

  init {
    val encodedConstants = this.core.getEncodedFSConstants().split('&')
    for (pair in encodedConstants) {
      val kv = pair.split('=')

      if (kv.size == 2) {
        val key = decodeURIComponent(kv[0])
        val value = decodeURIComponent(kv[1]).toInt()

        constants[key] = value
        android.util.Log.d(TAG, "Setting constant $key=$value")
      }
    }
  }

  public fun getNextAvailableId (): String {
    return (++this.nextId).toString()
  }

  public fun isAssetId (id: String): Boolean {
    if (openAssets[id] != null) {
      return true
    }

    return false
  }

  public fun access (
    seq: String = "",
    path: String,
    mode: Int,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    core.fsAccess(seq, path, mode, callbackId)
  }


  public fun open (
    seq: String = "",
    id: String,
    path: String,
    flags: Int,
    mode: Int,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    core.fsOpen(seq, id, path, flags, mode, callbackId)
  }

  public fun openAsset (
    id: String,
    path: String,
    callback: (String) -> Unit
  ) {
    val assetManager = core.getAssetManager()

    if (assetManager == null) {
      return callback(JSONError(id, "AssetManager is not initialized.").toString())
    }

    val fd = try {
      assetManager.openFd(path)
    } catch (err: Exception) {
      var message = err.toString()

      when (err) {
        is java.io.FileNotFoundException -> {
          message = "No such file or directory: ${err.message}"
        }
      }

      callback(JSONError(id, message).toString())

      null
    }

    if (fd == null) {
      return
    }

    if (!fd.getFileDescriptor().valid()) {
      callback(JSONError(id, "Invalid file descriptor.").toString())
      return
    }

    fd.close()

    this.openAssets[id] = path

    callback(JSONData(id, "\"fd\": ${id}").toString())
  }

  public fun close (
    seq: String = "",
    id: String,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    core.fsClose(seq, id, callbackId)
  }

  public fun closeAsset (
    id: String,
    callback: (String) -> Unit
  ) {
    if (this.openAssets[id] == null) {
      return callback(JSONError(id, "Invalid file descriptor.").toString())
    }

    this.openAssets.remove(id)
    return callback(JSONData(id).toString())
  }

  public fun read (
    seq: String = "",
    id: String,
    offset: Int = 0,
    size: Int,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    core.fsRead(seq, id, size, offset, callbackId)
  }

  public fun readAsset (
    id: String,
    offset: Int = 0,
    size: Int,
    callback: (String) -> Unit
  ) {
    val path = this.openAssets[id]

    if (path == null) {
      return callback(JSONError(id, "Invalid file descriptor.").toString())
    }

    val assetManager = core.getAssetManager()

    if (assetManager == null) {
      return callback(JSONError(id, "AssetManager is not initialized.").toString())
    }

    val fd = assetManager.openFd(path)
    val input = fd.createInputStream()
    val buffer = ByteArray(size)

    try {
      input.read(buffer, offset, size)
      val bytes = String(java.util.Base64.getEncoder().encode(buffer))
      callback(JSONData(id, "\"bytes\": \"$bytes\"").toString())
    } catch (err: Exception) {
      var message = err.toString()
      callback(JSONError(id, message).toString())
    }

    input.close()
    fd.close()
  }

  fun write () {
  }

  fun stat (
    seq: String = "",
    path: String,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    core.fsStat(seq, path, callbackId)
  }

  public fun fstat (
    seq: String = "",
    id: String,
    callback: (String) -> Unit
  ) {
    val callbackId = core.queueCallback(callback)
    android.util.Log.d(TAG, "$seq $id $callbackId")
    core.fsFStat(seq, id, callbackId)
  }

  fun unlink () {
  }

  fun rename () {
  }

  fun copyFile () {
  }

  fun rmdir () {
  }

  fun mkdir () {
  }

  fun readdir () {
  }
}

/**
 * NativeCore bindings externally implemented in JNI/NDK
 */
public open class NativeCore {
  protected val TAG = "NativeCore"

  /**
   * A reference to the activity that created this instance.
   */
  public var activity: WebViewActivity

  /**
   * Internal pointer managed by `initialize() and `destroy()
   */
  protected var pointer: Long = 0

  /**
   * @TODO
   */
  protected var nextCallbackId: Long = 0

  /**
   * @TODO
   */
  public var fs: NativeFileSystem

  /**
   * TODO
   */
  public val callbacks = mutableMapOf<String, (String) -> Unit>()

  /**
   * Set internally by the native binding if debug is enabled.
   */
  public var isDebugEnabled: Boolean = false

  /**
   * Set when all checks have passed.
   */
  public var isReady: Boolean = false

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
  external fun createPointer (): Long
  external fun destroyPointer (pointer: Long)

  /**
   * `NativeCore` vitals
   */
  @Throws(java.lang.Exception::class)
  external fun verifyFileSystem (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyLoop (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyRefs (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyJavaVM (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyPointer (pointer: Long): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyRootDirectory (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyAssetManager (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyNativeExceptions (): Boolean

  @Throws(java.lang.Exception::class)
  external fun verifyEnvironment (): Boolean

  /**
   * `NativeCore` internal utility bindings
   */
  @Throws(java.lang.Exception::class)
  external fun configureEnvironment (): Boolean

  @Throws(java.lang.Exception::class)
  external fun configureWebViewWindow (): Boolean

  @Throws(java.lang.Exception::class)
  external fun getPathToIndexHTML(): String

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource (): String

  @Throws(java.lang.Exception::class)
  external fun getResolveToRenderProcessJavaScript (
    seq: String,
    state: String,
    value: String,
    shouldEncodeValue: Boolean
  ): String

  @Throws(java.lang.Exception::class)
  external fun getEmitToRenderProcessJavaScript (
    event: String,
    value: String
  ): String

  @Throws(java.lang.Exception::class)
  external fun getStreamToRenderProcessJavaScript (
    id: String,
    value: String
  ): String

  /**
   * `NativeCore` bindings
   */
  @Throws(java.lang.Exception::class)
  external fun getNetworkInterfaces (): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformArch(): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformType(): String

  @Throws(java.lang.Exception::class)
  external fun getPlatformOS(): String

  @Throws(java.lang.Exception::class)
  external fun getEncodedFSConstants (): String

  @Throws(java.lang.Exception::class)
  external fun getFSConstants (): String

  @Throws(java.lang.Exception::class)
  external fun getPostData (id: String): ByteArray

  @Throws(java.lang.Exception::class)
  external fun freePostData (id: String)

  @Throws(java.lang.Exception::class)
  external fun fsAccess (
    seq: String,
    path: String,
    mode: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsClose (
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsFStat (
    seq: String,
    id: String,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsOpen (
    seq: String,
    id: String,
    path: String,
    flags: Int,
    mode: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsRead (
    seq: String,
    id: String,
    size: Int,
    offset: Int,
    callback: String
  )

  @Throws(java.lang.Exception::class)
  external fun fsStat (
    seq: String,
    path: String,
    callback: String
  )

  /**
   * `NativeCore` class constructor which is initialized
   * in the NDK/JNI layer.
   */
  constructor (activity: WebViewActivity) {
    this.activity = activity
    this.pointer = this.createPointer()
    this.fs = NativeFileSystem(this)
  }

  /**
   * Kotlin class internal destructor where we call `destroy()`.
   */
  protected fun finalize () {
    android.util.Log.d(TAG, "Destroying internal NativeCore pointer")
    this.destroyPointer(this.pointer)

    this.assetManager = null
    this.pointer = 0
  }

  /**
   * Configures `NativeCore` with Android dependencies.
   */
  @Throws(java.lang.Exception::class)
  public fun configure (config: NativeCoreConfiguration): Boolean {
    if (this.isReady) {
      return true
    }

    this.rootDirectory = config.rootDirectory
    this.assetManager = config.assetManager

    if (!this.configureEnvironment()) {
      return false
    }

    if (!this.configureWebViewWindow()) {
      return false
    }

    return true
  }

  /**
   * Returns the internal `AssetManager` instance.
   * @notice This function is used by the `NativeCore`
   */
  public fun getAssetManager (): android.content.res.AssetManager? {
    return this.assetManager
  }

  /**
   * Returns the internal rootDirectory string.
   * @notice This function is used by the `NativeCore`
   */
  public fun getRootDirectory (): String? {
    return this.rootDirectory
  }

  /**
   * Returns the next available callback ID
   */
  public fun getNextAvailableCallbackId (): String {
    return (this.nextCallbackId++).toString()
  }

  /**
   * @TODO
   */
  public fun evaluateJavascript (source: String) {
    this.activity.externalInterface?.evaluateJavascript(source, null)
  }

  /**
   * @TODO
   */
  public fun callback (id: String, data: String) {
    val cb = this.callbacks.get(id)

    if (cb != null) {
      cb.invoke(data)
      //this.callbacks.remove(id)
    }

    android.util.Log.d(TAG, "callback: id=$id data=$data")
  }

  /**
   * @TODO
   */
  public fun queueCallback (cb: (String) -> Unit): String {
    val id = this.getNextAvailableCallbackId()
    this.callbacks[id] = cb
    return id
  }

  /**
   * Performs internal vital checks.
   */
  public fun check (): Boolean {
    if (!this.verifyPointer(this.pointer)) {
      android.util.Log.e(TAG, "pointer check failed")
      return false
    } else {
      android.util.Log.d(TAG, "pointer check OK")
    }

    // @TODO
    if (!this.verifyRefs()) { return false }
    if (!this.verifyJavaVM()) { return false }

    try {
      this.verifyNativeExceptions()
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
    if (!this.verifyEnvironment()) { return false }

    try {
      if (!this.verifyRootDirectory()) {
        android.util.Log.e(TAG, "root directory check failed")
        return false
      }
      android.util.Log.d(TAG, "root directory check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "root directory check failed")
    }

    try {
      if (!this.verifyAssetManager()) {
        android.util.Log.e(TAG, "asset manager check failed")
        return false
      }
      android.util.Log.d(TAG, "asset manager check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "asset manager check failed")
    }

    if (!this.verifyLoop()) {
      android.util.Log.e(TAG, "libuv loop check failed")
      return false
    } else {
      android.util.Log.d(TAG, "libuv loop check OK")
    }

    try {
      if (!this.verifyFileSystem()) {
        android.util.Log.e(TAG, "filesystem check failed")
        return false
      }

      android.util.Log.d(TAG, "filesystem check OK")
    } catch (err: Exception) {
      android.util.Log.e(TAG, "filesystem check failed")
      android.util.Log.e(TAG, err.toString())

      return false
    }

    this.isReady = true
    return true
  }
}
