import { Deferred } from '../async/deferred.js'

const PlatformReadableStreamBYOBReader = globalThis.ReadableStreamBYOBReader
const PlatformReadableStreamBYOBRequest = globalThis.ReadableStreamBYOBRequest
const PlatformReadableByteStreamController = globalThis.ReadableByteStreamController

export class ReadableStream extends globalThis.ReadableStream {
  constructor (options) {
    if (options?.type === 'bytes') {
      delete options?.type
      console.warn(
        'ReadableStream: ReadableByteStreamController is not supported yet'
      )
    }

    super(options)
  }

  getReader (options) {
    if (options?.mode === 'byob') {
      return new ReadableStreamBYOBReader(this)
    }

    return super.getReader(options)
  }
}

class ReadableStreamBYOBReaderCancellation {
  reason = undefined
  state = false
}

class ReadableStreamBYOBReadResult {
  value = undefined
  done = false

  constructor (value = undefined, done = false) {
    this.value = value
    this.done = done
  }
}

export const ReadableStreamBYOBReader = PlatformReadableStreamBYOBReader || class ReadableStreamBYOBReader {
  #closed = new Deferred()
  #cancellation = new ReadableStreamBYOBReaderCancellation()

  get closed () {
    return this.#closed.promise
  }

  cancel (reason = undefined) {
    this.#cancellation.state = true
    if (typeof reason === 'string') {
      this.#cancellation.reason = reason
    }
    this.#closed.resolve(reason)
    return this.#closed.promise
  }

  read (view) {
    if (this.#cancellation.state === true) {
      return new ReadableStreamBYOBReadResult(undefined, true)
    }
  }
}

export const ReadableStreamBYOBRequest = PlatformReadableStreamBYOBRequest || class ReadableStreamBYOBRequest {
  #view = null

  get view () {
    return this.#view
  }

  respond (bytesWritten) {
  }

  respondWithNewView (view) {
  }
}

export const ReadableByteStreamController = PlatformReadableByteStreamController || class ReadableByteStreamController {
}

export default {
  ReadableStream
}
