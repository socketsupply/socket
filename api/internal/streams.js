export class ReadableStream extends globalThis.ReadableStream {
  constructor (options) {
    if (options?.type === 'bytes') {
      delete options?.type
    }

    super(options)
  }
}

export default {
  ReadableStream
}
