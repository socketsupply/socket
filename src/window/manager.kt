// vim: set sw=2:
package socket.runtime.window

import android.content.Context
import android.content.Intent
import android.graphics.drawable.ColorDrawable
import android.net.Uri
import android.os.Bundle
import android.view.WindowManager

import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.commit
import androidx.fragment.app.FragmentManager

import socket.runtime.app.App
import socket.runtime.core.console
import socket.runtime.window.WindowFragment
import socket.runtime.window.WindowOptions

import __BUNDLE_IDENTIFIER__.R

/**
 * A `WindowFragmentManager` manages `WindowFragment` instances.
 */
open class WindowFragmentManager (protected val activity: WindowManagerActivity) {
  open val fragments = mutableListOf<WindowFragment>()
  open val manager = activity.supportFragmentManager

  /**
   * Creates a new `WindowFragment` from `WindowOptions` if `options.index`
   * does not exist, otherwise this function is a "no-op".
   */
  fun createWindowFragment (options: WindowOptions) {
    val manager = this.manager
    if (!this.hasWindowFragment(options.index)) {
      val fragment = WindowFragment.newInstance(options)
      this.fragments.add(fragment)
      this.activity.runOnUiThread {
        manager.commit {
          // .setCustomAnimations(android.R.anim.slide_in_left, android.R.anim.slide_out_right)
          setReorderingAllowed(true)
          add(R.id.window, fragment)
          if (!options.headless) {
            //addToBackStack("window#${options.index}")
            addToBackStack(null)
          }
        }
      }
    }
  }

  /**
   * Shows a `WindowFragment` for a given `index`.
   * This function returns `true` if the `WindowFragment` exists and was shown,
   * otherwise `false`.
   */
  fun showWindowFragment (index: Int): Boolean {
    val fragment = this.fragments.find { it.index == index }

    if (fragment != null) {
      this.activity.runOnUiThread {
        this.manager.beginTransaction()
          // .setCustomAnimations(android.R.anim.slide_in_left, android.R.anim.slide_out_right)
          .show(fragment)
          .commit()
      }
      return true
    }

    return false
  }

  /**
   * Hides a `WindowFragment` for a given `index`.
   * This function returns `true` if the `WindowFragment` exists and was hidden,
   * otherwise `false`.
   */
  fun hideWindowFragment (index: Int): Boolean {
    val fragment = this.fragments.find { it.index == index }

    if (fragment != null) {
      this.activity.runOnUiThread {
        this.manager.beginTransaction()
        // .setCustomAnimations(android.R.anim.slide_in_left, android.R.anim.slide_out_right)
          .hide(fragment)
          .commit()
      }
      return true
    }

    return false
  }

  /**
   * Closes a `WindowFragment` for a given `index`.
   * This function returns `true` if the `WindowFragment` exists and was hidden,
   * otherwise `false`.
   */
  fun closeWindowFragment (index: Int): Boolean {
    val fragment = this.fragments.find { it.index == index }

    if (fragment != null) {
      this.fragments.remove(fragment)
      this.activity.runOnUiThread {
        this.manager.beginTransaction()
          // .setCustomAnimations(android.R.anim.slide_in_left, android.R.anim.slide_out_right)
          .remove(fragment)
          .commit()
      }
      return true
    }

    return false
  }

  /**
   * Returns `true` if there is a `WindowFragment` for the given `index`,
   * otherwise `false`.
   */
  fun hasWindowFragment (index: Int): Boolean {
    return this.fragments.find { it.index == index } != null
  }

  /**
   * Gets a `WindowFragment` at `index` if one exists, otherwise
   * `null` is returned.
   */
  fun getWindowFragment (index: Int): WindowFragment? {
    return this.fragments.find { it.index == index }
  }

  /**
   * "Pops" a `WindowFragment` from the back stack, navigating to the
   * previous `WindowFragment`.
   * This function returns `true` if a `WindowFragment` was "popped"
   * from the back stack, otherwise `false`.
   */
  fun popWindowFragment (): Boolean {
    if (this.manager.backStackEntryCount > 0) {
      this.manager.popBackStack()
      return true
    }

    this.activity.finish()
    return false
  }

  /**
   * Navigates a `WindowFragment` at a given `index` window's webview to
   * a given `url`
   * This function returns `true` if a `WindowFragment` did navigate to the
   * given `url`, otherwise `false`.
   */
  fun navigateWindowFragment (index: Int, url: String): Boolean {
    val fragment = this.fragments.find { it.index == index }
    val window = fragment?.window

    if (window != null) {
      window.navigate(url)
      return true
    }

    return false
  }

  /**
   * Evaluates a JavaScript `source` string at a given window `index`.
   */
  fun evaluateJavaScriptInWindowFragmentView (index: Int, source: String): Boolean {
    val fragments = this.fragments
    if (this.hasWindowFragment(index)) {
      kotlin.concurrent.thread {
        activity.runOnUiThread {
          val fragment = fragments.find { it.index == index }
          fragment?.webview?.evaluateJavascript(source, fun (_) {})
        }
      }
      return true
    }

    return false
  }

  /**
   * Gets the window fragment width.
   */
  fun getWindowFragmentWidth (index: Int): Int {
    val fragment = this.fragments.find { it.index == index }

    if (fragment != null) {
      return fragment.webview.measuredWidth
    }

    return 0
  }

  /**
   * Gets the window fragment height.
   */
  fun getWindowFragmentHeight (index: Int): Int {
    val fragment = this.fragments.find { it.index == index }

    if (fragment != null) {
      return fragment.webview.measuredHeight
    }

    return 0
  }

  /**
   * Sets the window fragment width and height.
   */
  fun setWindowFragmentSize (index: Int, width: Int, height: Int): Boolean {
    val fragment = this.fragments.find { it.index == index } ?: return false
    val window = fragment.window ?: return false
    window.setSize(width, height)
    return true
  }

  /**
   * Gets the window fragment title
   */
  fun getWindowFragmentTitle (index: Int): String {
    val fragment = this.fragments.find { it.index == index } ?: return ""
    val window = fragment.window ?: return ""
    return window.title
  }

  /**
   * Sets the window fragment title
   */
  fun setWindowFragmentTitle (index: Int, title: String): Boolean {
    val fragment = this.fragments.find { it.index == index } ?: return false
    val window = fragment.window ?: return false
    window.title = title
    return true
  }

  /**
   * XXX
   */
  fun setWindowFragmentViewBackgroundColor (index: Int, color: Long): Boolean {
    val fragment = this.fragments.find { it.index == index } ?: return false
    fragment.webview.setBackgroundColor(color.toInt())
    return true
  }

  /**
   * XXX
   */
  fun getWindowFragmentBackgroundColor (index: Int): Int {
    val fragment = this.fragments.find { it.index == index } ?: return 0
    val drawable = fragment.webview.background as ColorDrawable
    val color = drawable.color
    return 0xFFFFFF and color
  }

  /**
   * XXX
   */
  fun setWindowFragmentViewPosition(index: Int, x: Float, y: Float): Boolean {
    val fragment = this.fragments.find { it.index == index } ?: return false
    fragment.webview.setX(x)
    fragment.webview.setY(y)
    return true
  }
}

/**
 * The activity that is responsible for managing various WindowFragment
 * instances.
 */
open class WindowManagerActivity : AppCompatActivity(R.layout.window_container_view) {
  open val windowFragmentManager = WindowFragmentManager(this)

  override fun onBackPressed () {
    // this.windowFragmentManager.popWindowFragment()
  }

  /**
   * Creates a new window at a given `index`.
   */
  fun createWindow (
    index: Int = 0,
    shouldExitApplicationOnClose: Boolean = false,
    headless: Boolean = false
  ) {
    this.windowFragmentManager.createWindowFragment(WindowOptions(
      index = index,
      shouldExitApplicationOnClose = shouldExitApplicationOnClose,
      headless = headless
    ))
  }

  /**
   * Shows a window at a given `index`.
   */
  fun showWindow (index: Int): Boolean {
    return this.windowFragmentManager.showWindowFragment(index)
  }

  /**
   * Hides a window at a given `index`.
   */
  fun hideWindow (index: Int): Boolean {
    return this.windowFragmentManager.hideWindowFragment(index)
  }

  /**
   * Closes a window at a given `index`.
   */
  fun closeWindow (index: Int): Boolean {
    return this.windowFragmentManager.closeWindowFragment(index)
  }

  /**
   * Navigates to a given `url` for a window at a given `index`.
   */
  fun navigateWindow (index: Int, url: String): Boolean {
    return this.windowFragmentManager.navigateWindowFragment(index, url)
  }

  /**
   * Get the measured window width at a given `index`.
   */
  fun getWindowWidth (index: Int): Int {
    return this.windowFragmentManager.getWindowFragmentWidth(index)
  }

  /**
   * Get the measured window height at a given `index`.
   */
  fun getWindowHeight (index: Int): Int {
    return this.windowFragmentManager.getWindowFragmentHeight(index)
  }

  /**
   * Sets the window `width` and `height` at a given `index`.
   */
  fun setWindowFragmentSize (index: Int, width: Int, height: Int): Boolean {
    return this.windowFragmentManager.setWindowFragmentSize(index, width, height)
  }

  /**
   * XXX
   */
  fun getWindowBackgroundColor (index: Int): Int {
    return this.windowFragmentManager.getWindowFragmentBackgroundColor(index)
  }

  /**
   * Get the window title at a given `index`.
   */
  fun getWindowTitle (index: Int): String {
    return this.windowFragmentManager.getWindowFragmentTitle(index)
  }

  /**
   * Sets the window title at a given `index`.
   */
  fun setWindowTitle (index: Int, title: String): Boolean {
    return this.windowFragmentManager.setWindowFragmentTitle(index, title)
  }

  /**
   * Sets the window background color at a given `index`.
   */
  fun setWindowBackgroundColor (index: Int, color: Long): Boolean {
    return this.windowFragmentManager.setWindowFragmentViewBackgroundColor(index, color)
  }

  /**
   * XXX
   */
  fun setWindowPosition (index: Int, x: Float, y: Float): Boolean {
    return this.windowFragmentManager.setWindowFragmentViewPosition(index, x, y)
  }

  /**
   * Evaluates JavaScript source in a window at a given `index`.
   */
  fun evaluateJavaScript (index: Int, source: String): Boolean {
    return this.windowFragmentManager.evaluateJavaScriptInWindowFragmentView(index, source)
  }
}
