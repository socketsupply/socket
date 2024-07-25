package socket.runtime.window

import java.lang.Runtime

import android.net.Uri
import android.webkit.WebChromeClient

import androidx.activity.result.contract.ActivityResultContracts

import socket.runtime.core.console
import socket.runtime.window.WindowManagerActivity

/**
 * XXX
 */
open class Dialog (val activity: WindowManagerActivity) {
  /**
 * XXX
   */
  open class FileSystemPickerOptions (
    val params: WebChromeClient.FileChooserParams? = null,
    val mimeTypes: MutableList<String> = mutableListOf<String>(),
    val directories: Boolean = false,
    val multiple: Boolean = false,
    val files: Boolean = true
  ) {
    init {
      if (params != null && params.acceptTypes.size > 0) {
        this.mimeTypes += params.acceptTypes
      }
    }
  }

  var callback: ((results: Array<Uri>) -> Unit)? = null

  val launcherForSingleItem = activity.registerForActivityResult(
    ActivityResultContracts.GetContent(),
    fun (uri: Uri?) { this.resolve(uri) }
  )

  val launcherForMulitpleItems = activity.registerForActivityResult(
    ActivityResultContracts.GetMultipleContents(),
    { uris -> this.resolve(uris) }
  )

  // XXX(@jwerle): unused at the moment
  val launcherForSingleDocument = activity.registerForActivityResult(
    ActivityResultContracts.OpenDocument(),
    { uri -> this.resolve(uri) }
  )

  // XXX(@jwerle): unused at the moment
  val launcherForMulitpleDocuments = activity.registerForActivityResult(
    ActivityResultContracts.OpenMultipleDocuments(),
    { uris -> this.resolve(uris) }
  )

  // XXX(@jwerle): unused at the moment
  val launcherForSingleVisualMedia = activity.registerForActivityResult(
    ActivityResultContracts.PickVisualMedia(),
    { uri -> this.resolve(uri) }
  )

  // XXX(@jwerle): unused at the moment
  val launcherForMultipleVisualMedia = activity.registerForActivityResult(
    ActivityResultContracts.PickMultipleVisualMedia(),
    { uris -> this.resolve(uris) }
  )

  fun resolve (uri: Uri?) {
    if (uri != null) {
      return this.resolve(arrayOf(uri))
    }

    return this.resolve(arrayOf<Uri>())
  }

  fun resolve (uris: List<Uri>) {
    this.resolve(Array<Uri>(uris.size, { i -> uris[i] }))
  }

  fun resolve (uris: Array<Uri>) {
    val callback = this.callback
    if (callback != null) {
      this.callback = null
      callback(uris)
    }
  }

  fun showFileSystemPicker (
    options: FileSystemPickerOptions,
    callback: ((Array<Uri>) -> Unit)? = null
  ) {
    val mimeType =
      if (options.mimeTypes.size > 0 && options.mimeTypes[0].length > 0) {
        options.mimeTypes[0]
      } else { "*/*" }

    this.callback = callback

    this.activity.runOnUiThread {
      // TODO(@jwerle): support the other launcher types above
      // through the `showFileSystemPicker()` method some how
      if (options.multiple) {
        launcherForMulitpleItems.launch(mimeType)
      } else {
        launcherForSingleItem.launch(mimeType)
      }
    }
  }

  fun showFileSystemPicker (
    mimeTypes: String,
    directories: Boolean = false,
    multiple: Boolean = false,
    files: Boolean = true,
    pointer: Long = 0
  )  {
    return this.showFileSystemPicker(FileSystemPickerOptions(
      null,
      mimeTypes.split("|").toMutableList(),
      directories,
      multiple,
      files
    ), fun (uris: Array<Uri>) {
      if (pointer != 0L) {
        this.onResults(pointer, uris)
      }
    })
  }

  @Throws(Exception::class)
  external fun onResults (pointer: Long, results: Array<Uri>): Unit
}
