// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IRuntimeConfiguration {
  val rootDirectory: String
  val assetManager: android.content.res.AssetManager
  val evaluateJavascript: (String) -> Unit
}

data class RuntimeConfiguration (
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager,
  override val evaluateJavascript: (String) -> Unit
) : IRuntimeConfiguration

open class Runtime (
  activity: MainActivity? = null,
  configuration: RuntimeConfiguration
) {
  public var pointer = alloc()
  public var activity = WeakReference(activity)
  public val configuration = configuration;

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  external fun isDebugEnabled (): Boolean;
}
