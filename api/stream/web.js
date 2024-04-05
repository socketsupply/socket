import {
  ReadableStream,
  ReadableStreamBYOBReader,
  ReadableByteStreamController,
  ReadableStreamBYOBRequest,
  ReadableStreamDefaultController,
  ReadableStreamDefaultReader,

  WritableStream,
  WritableStreamDefaultController,
  WritableStreamDefaultWriter,

  TransformStream,
  TransformStreamDefaultController,

  ByteLengthQueuingStrategy,
  CountQueuingStrategy
} from '../internal/streams.js'

import * as exports from './web.js'

class UnsupportedStreamInterface {}

export {
  ReadableStream,
  ReadableStreamBYOBReader,
  ReadableByteStreamController,
  ReadableStreamBYOBRequest,
  ReadableStreamDefaultController,
  ReadableStreamDefaultReader,

  WritableStream,
  WritableStreamDefaultController,
  WritableStreamDefaultWriter,

  TransformStream,
  TransformStreamDefaultController,

  ByteLengthQueuingStrategy,
  CountQueuingStrategy
}

export const TextEncoderStream = globalThis.TextEncoderStream ?? UnsupportedStreamInterface
export const TextDecoderStream = globalThis.TextDecoderStream ?? UnsupportedStreamInterface
export const CompressionStream = globalThis.CompressionStream ?? UnsupportedStreamInterface
export const DecompressionStream = globalThis.DecompressionStream ?? UnsupportedStreamInterface

export default exports
