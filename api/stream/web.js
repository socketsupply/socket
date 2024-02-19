import * as exports from './web.js'

class UnsupportedStreamInterface {}

export const ReadableStream = globalThis.ReadableStream ?? UnsupportedStreamInterface
export const ReadableStreamDefaultReader = globalThis.ReadableStreamDefaultReader ?? UnsupportedStreamInterface
export const ReadableStreamBYOBReader = globalThis.ReadableStreamBYOBReader ?? UnsupportedStreamInterface
export const ReadableStreamBYOBRequest = globalThis.ReadableStreamBYOBRequest ?? UnsupportedStreamInterface
export const ReadableByteStreamController = globalThis.ReadableByteStreamController ?? UnsupportedStreamInterface
export const ReadableStreamDefaultController = globalThis.ReadableStreamDefaultController ?? UnsupportedStreamInterface
export const TransformStream = globalThis.TransformStream ?? UnsupportedStreamInterface
export const TransformStreamDefaultController = globalThis.TransformStreamDefaultController ?? UnsupportedStreamInterface
export const WritableStream = globalThis.WritableStream ?? UnsupportedStreamInterface
export const WritableStreamDefaultWriter = globalThis.WritableStreamDefaultWriter ?? UnsupportedStreamInterface
export const WritableStreamDefaultController = globalThis.WritableStreamDefaultController ?? UnsupportedStreamInterface
export const ByteLengthQueuingStrategy = globalThis.ByteLengthQueuingStrategy ?? UnsupportedStreamInterface
export const CountQueuingStrategy = globalThis.CountQueuingStrategy ?? UnsupportedStreamInterface
export const TextEncoderStream = globalThis.TextEncoderStream ?? UnsupportedStreamInterface
export const TextDecoderStream = globalThis.TextDecoderStream ?? UnsupportedStreamInterface
export const CompressionStream = globalThis.CompressionStream ?? UnsupportedStreamInterface
export const DecompressionStream = globalThis.DecompressionStream ?? UnsupportedStreamInterface

export default exports
