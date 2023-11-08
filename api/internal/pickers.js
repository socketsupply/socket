import application from '../application.js'
import mime from '../mime.js'
import path from '../path.js'
import fs from '../fs/promises.js'

/**
 * TODO
 */
export async function showOpenFilePicker (options = null) {
  const currentWindow = await application.getCurrentWindow()
  const handles = []

  const results = await currentWindow.showOpenFilePicker({
    allowMultiple: options?.multiple === true,
    allowFiles: true
  })

  for (const result of results) {
    const stats = await fs.stat(result)
    const types = await mime.lookup(path.extname(result).slice(1))
    const file = new File([], result, {
      lastModified: stats.mtimeMs,
      type: types[0]?.mime ?? ''
    })

    handles.push(Object.create(FileSystemFileHandle.prototype, {
      getFile: { value () { return file } }
    }))
  }

  return handles
}

export default {
  showOpenFilePicker
}
