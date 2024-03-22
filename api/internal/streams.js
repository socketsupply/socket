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
}

export default {
  ReadableStream
}
