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
 * The native platform `WebAssembly.compileStreaming()` function.
 * @ignore
 */
const nativeCompileStreaming = globalThis
  .WebAssembly
  .compileStreaming
  .bind(globalThis.WebAssembly)

/**
 * The `instantiateStreaming()` function compiles and instantiates a WebAssembly
 * module directly from a streamed source.
 * @ignore
 * @param {Response} response
 * @param {=object} [importObject]
 * @return {Promise<WebAssembly.Instance>}
 */
export async function instantiateStreaming (response, importObject = undefined) {
  const result = await response

  if (
    result instanceof __native_Response &&
    result.constructor === __native_Response
  ) {
    return nativeInstantiateStreaming(result, importObject)
  }

  const stream = await (result.body || result._bodyInit)
  const nativeResponse = new __native_Response(stream, {
    statusText: response.statsText,
    headers: response.headers,
    status: response.status
  })

  return nativeInstantiateStreaming(nativeResponse, importObject)
}

/**
 * The `compileStreaming()` function compiles and instantiates a WebAssembly
 * module directly from a streamed source.
 * @ignore
 * @param {Response} response
 * @return {Promise<WebAssembly.Module>}
 */
export async function compileStreaming (response) {
  const result = await response

  if (
    result instanceof __native_Response &&
    result.constructor === __native_Response
  ) {
    return nativeCompileStreaming(result)
  }

  const stream = await (result.body || result._bodyInit)
  const nativeResponse = new __native_Response(stream, {
    statusText: response.statsText,
    headers: response.headers,
    status: response.status
  })

  return nativeCompileStreaming(nativeResponse)
}

export default { instantiateStreaming }
