package socket.runtime.debug

object console {
  val TAG = "Console"
  fun log (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun info (string: String) {
    android.util.Log.i(TAG, string)
  }

  fun debug (string: String) {
    android.util.Log.d(TAG, string)
  }

  fun error (string: String) {
    android.util.Log.e(TAG, string)
  }
}
