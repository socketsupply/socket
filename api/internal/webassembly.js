/* global __native_Response */
/**
 * The native platform `WebAssembly.instantiateStreaming()` function.
 * @ignore
 */
const nativeInstantiateStreaming = globalThis
  .WebAssembly
  .instantiateStreaming
  .bind(globalThis.WebAssembly)

/**
 * The `instantiateStreaming()` function compiles and instantiates a WebAssembly
 * module directly from a streamed source.
 * @ignore
 * @param {Response}
 * @param
 */
export async function instantiateStreaming (response, importObject = null) {
  const result = await response
  const stream = await (result.body || result._bodyInit)
  const nativeResponse = new __native_Response(stream, {
    statusText: response.statsText,
    headers: response.headers,
    status: response.status
  })

  return nativeInstantiateStreaming(nativeResponse, importObject)
}

export default { instantiateStreaming }
