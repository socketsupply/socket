// vim: set sw=2:
package socket.runtime.webview

import socket.runtime.bridge.Bridge

open class Navigator (val bridge: Bridge) {
  fun isNavigationRequestAllowed (
    currentURL: String,
    requestedURL: String
  ): Boolean {
    return this.isNavigationRequestAllowed(
      this.bridge.index,
      currentURL,
      requestedURL
    )
  }

  @Throws(Exception::class)
  external fun isNavigationRequestAllowed (
    index: Int,
    currentURL: String,
    requestedURL: String
  ): Boolean
}
