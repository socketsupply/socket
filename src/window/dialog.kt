package socket.runtime.window

import java.lang.Runtime
import java.util.concurrent.Semaphore

import android.net.Uri
import android.webkit.WebChromeClient

import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity

/**
 */
open class Dialog (val window: Window) {
  /**
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

  var results = arrayOf<Uri>()
  val activity = window.activity as AppCompatActivity
  val semaphore = Semaphore(1)

  fun resolve (uri: Uri?) {
    if (uri != null) {
      this.results = arrayOf(uri)
    }
    this.semaphore.release()
  }

  fun resolve (uris: Array<Uri>) {
    this.results = uris
    this.semaphore.release()
  }

  fun resolve (uris: List<Uri>) {
    this.results = Array<Uri>(uris.size, { i -> uris[i] })
    this.semaphore.release()
  }

  fun showFileSystemPicker (options: FileSystemPickerOptions): Array<Uri> {
    val mimeType =
      if (options.mimeTypes.size > 0) { options.mimeTypes[0] }
      else { "*/*" }

    this.semaphore.acquireUninterruptibly()

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

    // TODO(@jwerle): support the other launcher types above
    // through the `showFileSystemPicker()` method some how

    if (options.multiple) {
      launcherForMulitpleItems.launch(mimeType)
    } else {
      launcherForSingleItem.launch(mimeType)
    }

    this.semaphore.acquireUninterruptibly()
    val results = this.results
    this.semaphore.release()
    return results
  }

  fun showFileSystemPicker (
    mimeTypes: String,
    directories: Boolean = false,
    multiple: Boolean = false,
    files: Boolean = true
  ): Array<Uri> {
    return this.showFileSystemPicker(FileSystemPickerOptions(
      null,
      mimeTypes.split("|").toMutableList(),
      directories,
      multiple,
      files
    ))
  }
}
