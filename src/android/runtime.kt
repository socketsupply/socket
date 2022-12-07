// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IRuntimeConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
  val evaluateJavascript: (String) -> Unit
  val exit: (Int) -> Unit
  val openExternal: (String) -> Unit
}

data class RuntimeConfiguration (
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager,
  override val evaluateJavascript: (String) -> Unit,
  override val exit: (Int) -> Unit,
  override val openExternal: (String) -> Unit
) : IRuntimeConfiguration

open class Runtime (
  activity: MainActivity,
  configuration: RuntimeConfiguration
) {
  public var pointer = alloc(activity.getRootDirectory())
  public var activity = WeakReference(activity)
  public val configuration = configuration;

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  public fun evaluateJavascript (source: String) {
    this.configuration.evaluateJavascript(source)
  }

  public fun exit (code: Int) {
    this.configuration.exit(code)
  }

  public fun openExternal (value: String) {
    this.configuration.openExternal(value)
  }

  public fun start () {
    this.resume()
  }

  public fun stop () {
    this.pause()
  }

  public fun destroy () {
    this.stop()
    this.finalize()
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
}
