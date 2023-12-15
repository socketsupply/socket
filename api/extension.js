/* global Event */
/**
 * @module extension
 *
 * @example
 * import extension from 'socket:extension'
 * const ex = await extension.load('native_extension')
 * const result = ex.binding.method('argument')
 */
import { isBufferLike } from './util.js'
import { createFile } from './fs/web.js'
// import { Buffer } from './buffer.js'
import process from './process.js'
import crypto from './crypto.js'
import ipc from './ipc.js'
import fs from './fs/promises.js'

const $loaded = Symbol('loaded')
const $stats = Symbol('stats')
const $type = Symbol('type')

/**
 * A constructed `Response` suitable for WASM binaries.
 * @ignore
 */
class WebAssemblyResponse extends Response {
  constructor (stream) {
    super(stream, {
      headers: {
        'content-type': 'application/wasm'
      }
    })
  }
}

class WebAssemblyExtensionStack {
  constructor (adapter) {
    this.adapter = adapter
    this.limits = {
      min: this.adapter.instance.exports.__stack_low.value,
      max: this.adapter.instance.exports.__stack_high.value
    }

    this.offset = this.limits.min
    this.current = []
  }

  get buffer () {
    return this.adapter.buffer
  }

  get (pointer) {
    return this.buffer.subarray(pointer)
  }

  push (value) {
    const pointer = this.offset

    if (value === null) {
      value = 0
    }

    if (typeof value === 'string') {
      value = this.adapter.textEncoder.encode(value)
    } else if (isBufferLike(value)) {
      value = new Uint8Array(value)
    }

    if (!isBufferLike(value) && typeof value !== 'number') {
      throw new TypeError(
        'WebAssemblyExtensionStack: ' +
        'Expected value to be TypedArray or a number: ' +
        `Got ${typeof value}`
      )
    }

    let byteLength = 0
    const isFloat = typeof value === 'number' && parseInt(value) !== value

    if (isBufferLike(value)) {
      byteLength = value.byteLength
    } else {
      const zeroes = Math.clz32(value)
      if (zeroes <= 8) {
        byteLength = 4
      } else if (zeroes <= 16) {
        byteLength = 2
      } else if (zeroes <= 32) {
        byteLength = 1
      }
    }

    this.current.push(byteLength)
    this.offset += byteLength

    if (isBufferLike(value)) {
      this.adapter.set(pointer, value)
    } else if (byteLength === 4) {
      if (isFloat) {
        this.adapter.setFloat32(pointer, value)
      } else {
        this.adapter.setInt32(pointer, value)
      }
    } else if (byteLength === 2) {
      if (isFloat) {
        this.adapter.setFloat16(pointer, value)
      } else {
        this.adapter.setInt16(pointer, value)
      }
    } else if (byteLength === 1) {
      if (isFloat) {
        this.adapter.setFloat8(pointer, value)
      } else {
        this.adapter.setInt8(pointer, value)
      }
    }

    return pointer
  }

  pop () {
    if (this.current.length === 0) {
      return 0
    }

    const byteLength = this.current.pop()
    const pointer = this.offset - byteLength
    this.offset -= byteLength
    return pointer
  }

  rewind (offset) {
    const pointers = []
    while (this.offset > offset) {
      pointers.push(this.pop())
    }
    return pointers
  }
}

class WebAssemblyExtensionHeap {
  constructor (adapter) {
    this.adapter = adapter
  }

  get buffer () {
    return this.adapter.buffer
  }

  get (pointer) {
    return this.buffer.subarray(pointer)
  }
}

class WebAssemblyExtensionIndirectFunctionTable {
  constructor (adapter, options = null) {
    this.adapter = adapter
    this.table = (
      options?.table ??
      adapter.instance?.exports?.__indirect_function_table ??
      new WebAssembly.Table({ initial: 0, element: 'anyfunc' })
    )
  }

  call (index, ...args) {
    if (this.table.length === 0 || index >= this.table.length) {
      return null
    }

    const callable = this.table.get(index)
    const values = []
    const mark = this.adapter.stack.offset

    for (const arg of args) {
      if (typeof arg === 'number') {
        values.push(arg)
      } else {
        values.push(this.adapter.stack.push(arg))
      }
    }

    const value = callable(...values)
    this.adapter.stack.rewind(mark)

    return value
  }
}

/**
 * An adapter for reading and writing various values from a WebAssembly instance's
 * memory buffer.
 * @ignore
 */
class WebAssemblyExtensionAdapter {
  constructor ({ instance, module, table }) {
    this.view = new DataView(instance.exports.memory.buffer)
    this.table = table
    this.buffer = new Uint8Array(instance.exports.memory.buffer)
    this.module = module
    this.instance = instance
    this.textDecoder = new TextDecoder()
    this.textEncoder = new TextEncoder()

    this.indirectFunctionTable = new WebAssemblyExtensionIndirectFunctionTable(this)
    this.stack = new WebAssemblyExtensionStack(this)
    this.heap = new WebAssemblyExtensionHeap(this)
  }

  init () {
    this.instance.exports.__wasm_call_ctors()
    const offset = this.instance.exports.__sapi_extension_initializer()
    const context = new Uint8Array(1)
    return Boolean(this.indirectFunctionTable.call(offset, context))
  }

  get globalBaseOffset () {
    return this.instance.exports.__global_base.value
  }

  get (pointer) {
    if (!pointer) {
      return null
    }

    return this.buffer.subarray(pointer)
  }

  set (pointer, value) {
    if (pointer) {
      if (typeof value === 'string') {
        value = this.textEncoder.encode(value)
      }

      this.buffer.set(value, pointer)
    }
  }

  getFloat32 (pointer) {
    return pointer ? this.view.getFloat32(pointer, true) : 0
  }

  setFloat32 (pointer, value) {
    return pointer ? (this.view.setFloat32(pointer, value, true), true) : false
  }

  getInt8 (pointer) {
    return pointer ? this.view.getInt8(pointer, true) : 0
  }

  setInt8 (pointer, value) {
    return pointer ? (this.view.setInt8(pointer, value, true), true) : false
  }

  getInt16 (pointer) {
    return pointer ? this.view.getInt16(pointer, true) : 0
  }

  setInt16 (pointer, value) {
    return pointer ? (this.view.setInt16(pointer, value, true), true) : false
  }

  getInt32 (pointer) {
    return pointer ? this.view.getInt32(pointer, true) : 0
  }

  setInt32 (pointer, value) {
    return pointer ? (this.view.setInt32(pointer, value, true), true) : false
  }

  getUint8 (pointer) {
    return pointer ? this.view.getUint8(pointer, true) : 0
  }

  setUint8 (pointer, value) {
    return pointer ? (this.view.setUint8(pointer, value, true), true) : false
  }

  getUint16 (pointer) {
    return pointer ? this.view.getUint16(pointer, true) : 0
  }

  setUint16 (pointer, value) {
    return pointer ? (this.view.setUint16(pointer, value, true), true) : false
  }

  getUint32 (pointer) {
    return pointer ? this.view.getUint32(pointer, true) : 0
  }

  setUint32 (pointer, value) {
    return pointer ? (this.view.setUint32(pointer, value, true), true) : false
  }

  getString (pointer, buffer, size) {
    if (!pointer) {
      return null
    }

    if (typeof buffer === 'number') {
      size = buffer
      buffer = null
    }

    const start = buffer
      ? buffer.slice(pointer)
      : this.stack.get(pointer)

    const end = size || start.indexOf(0) // NULL byte

    if (end === -1) {
      return this.textDecoder.decode(start)
    }

    return this.textDecoder.decode(start.slice(0, end))
  }

  setString (pointer, string, buffer) {
    if (pointer) {
      const encoded = this.textEncoder.encode(string)
      ;(buffer || this.buffer).set(encoded, pointer)
      return true
    }

    return false
  }
}

/**
 * A container for a native WebAssembly extension info.
 * @ignore
 */
class WebAssemblyExtensionInfo {
  constructor (adapter) {
    this.adapter = adapter
  }

  get instance () {
    return this.adapter.instance
  }

  get abi () {
    return this.adapter.instance.exports.__sapi_extension_abi()
  }

  get name () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_name())
  }

  get version () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_version())
  }

  get description () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_description())
  }
}

function createWebAssemblyExtensionBinding (adapter) {
  return new Proxy(adapter, {
    get (target, key) {
      if (typeof adapter.instance.exports[key] === 'function') {
        return adapter.instance.exports[key].bind(adapter.instance.exports)
      }

      // TODO: invoke IPC route?
    }
  })
}

function createWebAssemblyExtensionImports (env) {
  const imports = {
    env: {
      memoryBase: 0,
      tableBase: 0
    }
  }

  imports.table = env.table || new WebAssembly.Table({
    initial: 0,
    element: 'anyfunc'
  })

  // <assert.h>
  Object.assign(imports.env, {
    __assert_fail (condition, filename, line, func) {
    }
  })

  // <errno.h>
  Object.assign(imports.env, {
    __errno_location () {
      // TODO(@jwerle): return pointer from `env.adapter.heap` that is in reserved space
      return 0
    }
  })

  function variadicFormatedStringFromPointers (
    formatPointer,
    variadicArguments = 0x0,
    encoded = true
  ) {
    const format = env.adapter.getString(formatPointer)
    if (!variadicArguments) {
      if (encoded) {
        return env.adapter.textEncoder.encode(format)
      }

      return format
    }

    const buffer = env.adapter.buffer.slice(variadicArguments)
    const view = new DataView(buffer.buffer)

    let index = 0

    const regex = /%(l|ll|j|t|z)?(d|i|o|s|S|u|x|X|n|%)/g
    const output = format.replace(regex, (x) => {
      if (x === '%%') {
        return '%'
      }

      switch (x) {
        case '%S': case '%s': {
          const pointer = view.getUint8((index++) * 4)
          const offset = env.adapter.globalBaseOffset + pointer
          return env.adapter.getString(offset, env.adapter.buffer)
        }

        case '%llu': case '%lu':
        case '%lld': case '%ld':
        case '%i': case '%x': case '%X': case '%d': {
          return view.getInt32((index++) * 4, true)
        }

        case '%u': {
          return view.getUint32((index++) * 4, true)
        }
      }

      return x
    })

    if (encoded) {
      return env.adapter.textEncoder.encode(output)
    }

    return output
  }

  // eslint-disable-next-line
  const STDIN = 0x0
  const STDOUT = 0x1
  const STDERR = 0x2

  const toBeFlushed = {
    stdout: [],
    stderr: []
  }

  // <stdio.h>
  Object.assign(imports.env, {
    printf (formatPointer, variadicArguments) {
      return imports.env.fprintf(STDOUT, formatPointer, variadicArguments)
    },

    fprintf (descriptorPointer, formatPointer, variadicArguments) {
      const output = variadicFormatedStringFromPointers(
        formatPointer,
        variadicArguments,
        /* encoded = */ false
      )

      if (descriptorPointer === STDOUT) {
        toBeFlushed.stdout.push(output)
      } else if (descriptorPointer === STDERR) {
        toBeFlushed.stderr.push(output)
      }

      if (output.includes('\n')) {
        if (descriptorPointer === STDOUT) {
          console.log(toBeFlushed.stdout.join(''))
          toBeFlushed.stdout.splice(0, toBeFlushed.stdout.length)
        } else if (descriptorPointer === STDERR) {
          console.error(toBeFlushed.stderr.join(''))
          toBeFlushed.stderr.splice(0, toBeFlushed.stderr.length)
        }
      }

      return output.length
    },

    sprintf (stringPointer, formatPointer, variadicArguments) {
      const output = variadicFormatedStringFromPointers(formatPointer, variadicArguments)

      if (stringPointer !== 0) {
        const buffer = env.adapter.get(stringPointer)
        buffer.set(output)
      }

      return output.byteLength
    },

    snprintf (stringPointer, size, formatPointer, variadicArguments) {
      const output = variadicFormatedStringFromPointers(formatPointer, variadicArguments)

      if (stringPointer !== 0 && size !== 0) {
        const buffer = env.adapter.get(stringPointer)
        buffer.set(output.slice(0, size - 1))
      }

      return output.byteLength
    }
  })

  // <string.h>
  Object.assign(imports.env, {
    memcpy (dst, src, size) {
    },

    memmove (dst, src, size) {
    },

    memset (dst, byte, size) {
    },

    memcmp (left, right, size) {
    },

    memchr (string, char, size) {
    },

    strcpy (dst, src) {
    },

    strncpy (dst, src, size) {
    },

    strcat (dst, src) {
    },

    strncat (dst, src, size) {
    },

    strcmp (left, right) {
    },

    strncmp (left, right, size) {
    },

    strcoll (left, right) {
    },

    strxfrm (dst, src, size) {
    },

    strchr (string, char) {
    },

    strrchr (string, char) {
    },

    strcspn (string, charset) {
    },

    strspn (string, charset) {
    },

    strpbrk (string, charset) {
    },

    strstr (string, needle) {
    },

    strtok (string, sep) {
    },

    strlen (string) {
    },

    strdup (string) {
    },

    strndup (string, size) {
    },

    strerror (errorCode) {
    },

    strsignal (signal) {
    }
  })

  // <socket/extension.h>
  Object.assign(imports.env, {
    // utils
    sapi_log (ctx, message) {
      const string = env.adapter.getString(message)
      console.log(string)
    },

    sapi_debug (ctx, message) {
      const string = env.adapter.getString(message)
      console.debug(string)
    },

    sapi_rand64 () {
      return crypto.rand64()
    },

    // extension
    sapi_extension_register (pointer) {
    },

    // context
    sapi_context_create (parent, retained) {
    },

    sapi_context_retain (context) {
    },

    sapi_context_retained (context) {
    },

    sapi_context_get_loop (context) {
      return 0
    },

    sapi_context_release (context) {
    },

    sapi_context_set_data (context, data) {
    },

    sapi_context_get_data (context) {
    },

    sapi_context_dispatch (context, data, callback) {
      if (callback > 0) {
        process.nextTick(() => {
          env.adapter.indirectFunctionTable.call(callback, context, data)
        })
      }
    },

    sapi_context_alloc (context, size) {
    },

    // env
    sapi_env_get (context, name) {
      name = env.adapter.getString(name)

      if (!process.env[name]) {
        return 0
      }

      return env.adapter.stack.push(process.env[name])
    },

    // javascript
    async sapi_javascript_evaluate (context, name, source) {
      name = env.adapter.getString(name)
      source = env.adapter.getString(source)
      // TODO(@jwerle): implement a 'vm' module and use it
      // eslint-disable-next-line
      const evaluation = new Function(`
        return function () { ${source}; }
        //# sourceURL=${name}
      `)()
      try {
        await evaluation()
      } catch (err) {
        console.error('sapi_javascript_evaluate:', err)
      }
    },

    // JSON
    sapi_json_typeof (value) {
    },

    sapi_json_object_create (context) {
    },

    sapi_json_array_create (context) {
    },

    sapi_json_string_create (context, string) {
    },

    sapi_json_boolean_create (context, boolean) {
    },

    sapi_json_number_create (context, number) {
    },

    sapi_json_raw_from (context, source) {
    },

    sapi_json_object_set_value (object, key, value) {
    },

    sapi_json_object_get (object, key) {
    },

    sapi_json_array_set_value (array, index, value) {
    },

    sapi_json_array_get (array, index) {
    },

    sapi_json_array_push_value (array, value) {
    },

    sapi_json_array_pop (array) {
    },

    sapi_json_stringify_value (value) {
    }

    // TODO: IPC
  })

  return imports
}

/**
 * @typedef {{
 *   allow: string[] | string,
 *   imports?: object,
 *   type?: 'shared' | 'wasm32',
 *   instance?: WebAssembly.Instance,
 *   adapter?: WebAssemblyExtensionAdapter
 * }} ExtensionLoadOptions
 */

/**
 * @typedef {{ abi: number, version: string, description: string }} ExtensionInfo
 */

/**
 * @typedef {{ abi: number, loaded: number }} ExtensionStats
 */

/**
 * A interface for a native extension.
 * @template {Record<string, any> T}
 */
export class Extension extends EventTarget {
  /**
   * Load an extension by name.
   * @template {Record<string, any> T}
   * @param {string} name
   * @param {ExtensionLoadOptions} [options]
   * @return {Promise<Extension<T>>}
   */
  static async load (name, options) {
    options = { name, ...options }

    if (Array.isArray(options.allow)) {
      options.allow = options.allow.join(',')
    }

    options.type = await this.type(name)
    const stats = await this.stats(name)

    let info = null

    if (options.type === 'wasm32') {
      let adapter = null

      const path = stats.path.startsWith('/') ? stats.path.slice(1) : stats.path
      const fd = await fs.open(path)
      const file = await createFile(path, { fd })
      const stream = file.stream()
      const response = new WebAssemblyResponse(stream)
      const table = new WebAssembly.Table({
        initial: 0,
        element: 'anyfunc'
      })

      const imports = {
        ...options.imports,
        ...createWebAssemblyExtensionImports({
          table,
          get adapter () {
            return adapter
          }
        })
      }

      const {
        instance,
        module
      } = await WebAssembly.instantiateStreaming(response, imports)

      adapter = new WebAssemblyExtensionAdapter({ instance, module, table })
      info = new WebAssemblyExtensionInfo(adapter)

      options.adapter = adapter
      options.instance = instance

      adapter.init()
    } else {
      const result = await ipc.request('extension.load', options)
      if (result.err) {
        throw new Error('Failed to load extensions', { cause: result.err })
      }

      info = result.data
    }

    const extension = new this(name, info, options)
    extension[$stats] = stats
    extension[$loaded] = true
    return extension
  }

  /**
   * Query type of extension by name.
   * @param {string} name
   * @return {Promise<'shared'|'wasm32'|'unknown'|null>}
   */
  static async type (name) {
    const result = await ipc.request('extension.type', { name })

    if (result.err) {
      throw result.err
    }

    return result.data.type ?? null
  }

  /**
   * Provides current stats about the loaded extensions or one by name.
   * @param {?string} name
   * @return {Promise<ExtensionStats|null>}
   */
  static async stats (name) {
    const result = await ipc.request('extension.stats', { name: name || '' })
    if (result.err) {
      throw result.err
    }
    return result.data ?? null
  }

  /**
   * The name of the extension
   * @type {string?}
   */
  name = null

  /**
   * The version of the extension
   * @type {string?}
   */
  version = null

  /**
   * The description of the extension
   * @type {string?}
   */
  description = null

  /**
   * The abi of the extension
   * @type {number}
   */
  abi = 0

  /**
   * @type {object}
   */
  options = {}

  /**
   * @type {T}
   */
  binding = null

  /**
   * Not `null` if extension is of type 'wasm32'
   * @type {?WebAssembly.Instance}
   */
  instance = null

  /**
   * Not `null` if extension is of type 'wasm32'
   * @type {?WebAssemblyExtensionAdapter}
   */
  adapter = null

  /**
   * `Extension` class constructor.
   * @param {string} name
   * @param {ExtensionInfo} info
   * @param {ExtensionLoadOptions} [options]
   */
  constructor (name, info, options = null) {
    super()

    this[$type] = options?.type || 'shared'
    this[$loaded] = false

    this.abi = info?.abi || 0
    this.name = name
    this.version = info?.version || null
    this.description = info?.description || null
    this.options = options ?? {}

    if (this.type === 'shared') {
      this.binding = ipc.createBinding(options?.binding?.name || this.name, {
        ...options?.binding,
        extension: this,
        default: 'request'
      })
    } else if (this.type === 'wasm32') {
      this.instance = options?.instance
      this.adapter = options?.adapter
      this.binding = createWebAssemblyExtensionBinding(this.adapter)
    }
  }

  /**
   * `true` if the extension was loaded, otherwise `false`
   * @type {boolean}
   */
  get loaded () {
    return this[$loaded]
  }

  /**
   * The extension type: 'shared' or  'wasm32'
   * @type {'shared'|'wasm32'}
   */
  get type () {
    return this[$type]
  }

  /**
   * Unloads the loaded extension.
   * @throws Error
   */
  async unload () {
    if (!this.loaded) {
      throw new Error('Extension is not loaded')
    }

    if (this.type === 'shared') {
      const { name } = this
      const result = await ipc.request('extension.unload', { name })

      if (result.err) {
        throw new Error('Failed to unload extension', { cause: result.err })
      }
    } else if (this.type === 'wasm32') {
      // remove references
      this.instance = null
      this.adapter = null
      this.binding = null
    }

    this[$loaded] = false
    this.dispatchEvent(new Event('unload'))

    return true
  }
}

/**
 * Load an extension by name.
 * @template {Record<string, any> T}
 * @param {string} name
 * @param {ExtensionLoadOptions} [options]
 * @return {Promise<Extension<T>>}
 */
export async function load (name, options = {}) {
  return await Extension.load(name, options)
}

/**
 * Provides current stats about the loaded extensions.
 * @return {Promise<ExtensionStats>}
 */
export async function stats () {
  return await Extension.stats()
}

export default {
  load,
  stats
}
