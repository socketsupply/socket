import diagnostics from '../diagnostics.js'
import console from '../console.js'
import ipc from '../ipc.js'

const dc = diagnostics.channels.group('fs', [
  'fd',
  'fd.retain',
  'fd.release'
])

/**
 * Static contsiner to map file descriptors to internal
 * identifiers with type reflection.
 */
export default new class FileDescriptorsMap {
  types = new Map()
  fds = new Map()
  ids = new Map()

  constructor () {
    this.syncOpenDescriptors()
  }

  get size () {
    return this.ids.size
  }

  get (id) {
    return this.fds.get(id)
  }

  async syncOpenDescriptors () {
    // wait for DOM to be loaded and ready
    if (typeof globalThis.document === 'object') {
      if (globalThis.document.readyState !== 'complete') {
        await new Promise((resolve) => {
          document.addEventListener('DOMContentLoaded', resolve, { once: true })
        })
      }
    }

    const result = ipc.sendSync('fs.getOpenDescriptors')
    if (Array.isArray(result.data)) {
      for (const { id, fd, type } of result.data) {
        this.set(id, fd, type)
      }
    }
  }

  set (id, fd, type) {
    if (!type) {
      type = id === fd ? 'directory' : 'file'
    }

    if (!this.has(id)) {
      dc.channel('fd').publish({ fd })

      this.fds.set(id, fd)
      this.ids.set(fd, id)
      this.types.set(id, type)
      this.types.set(fd, type)
    }
  }

  has (id) {
    return this.fds.has(id) || this.ids.has(id)
  }

  fd (id) {
    return this.get(id)
  }

  id (fd) {
    return this.ids.get(fd)
  }

  async release (id, closeDescriptor = true) {
    let result = null

    if (id === undefined) {
      this.clear()

      if (closeDescriptor !== false) {
        result = await ipc.send('fs.closeOpenDescriptors')

        if (result.err && !/found/i.test(result.err.message)) {
          console.warn('fs.fds.release', result.err.message || result.err)
        }
      }

      return
    }

    id = this.ids.get(id) || id

    const fd = this.fds.get(id)

    if (fd) {
      dc.channel('fd.release').publish({ fd })
    }

    this.fds.delete(id)
    this.fds.delete(fd)

    this.ids.delete(fd)
    this.ids.delete(id)

    this.types.delete(id)
    this.types.delete(fd)

    if (closeDescriptor !== false) {
      result = await ipc.send('fs.closeOpenDescriptor', { id: String(id) })

      if (result.err && !/found/i.test(result.err.message)) {
        console.warn('fs.fds.release', result.err.message || result.err)
      }
    }
  }

  async retain (id) {
    const result = await ipc.send('fs.retainOpenDescriptor', { id })
    const fd = this.fd(id)

    if (result.err) {
      throw result.err
    }

    if (fd) {
      dc.channel('fd.retain').publish({ fd })
    }

    return result.data
  }

  delete (id) {
    this.release(id)
  }

  clear () {
    this.ids.clear()
    this.fds.clear()
    this.types.clear()
  }

  typeof (id) {
    return this.types.get(id) || this.types.get(this.fds.get(id))
  }

  entries () {
    return this.ids.entries()
  }
}()
