// vim: set sw=2:
package __BUNDLE_IDENTIFIER__

open class App : socket.runtime.app.App() {
  companion object {
    init {
      socket.runtime.app.App.loadSocketRuntime()
    }
  }
}
