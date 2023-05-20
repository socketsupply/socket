import { IllegalConstructorError } from './errors.js'
import { Buffer } from './buffer.js'

import * as exports from './util.js'

const ObjectPrototype = Object.prototype
const Uint8ArrayPrototype = Uint8Array.prototype
const TypedArrayPrototype = Object.getPrototypeOf(Uint8ArrayPrototype)

const AsyncFunction = (async () => {}).constructor
const TypedArray = TypedArrayPrototype.constructor

const kSocketCustomInspect = inspect.custom = Symbol.for('socket.util.inspect.custom')
const kNodeCustomInspect = inspect.custom = Symbol.for('nodejs.util.inspect.custom')
const kIgnoreInspect = inspect.ignore = Symbol.for('socket.util.inspect.ignore')

function maybeURL (...args) {
  try {
    return new URL(...args)
  } catch (_) {
    return null
  }
}

export function hasOwnProperty (object, property) {
  return ObjectPrototype.hasOwnProperty.call(object, String(property))
}

export function isTypedArray (object) {
  return object instanceof TypedArray
}

export function isArrayLike (object) {
  return (
    (Array.isArray(object) || isTypedArray(object)) &&
    object !== TypedArrayPrototype &&
    object !== Buffer.prototype
  )
}

export function isArrayBufferView (buf) {
  return !Buffer.isBuffer(buf) && ArrayBuffer.isView(buf)
}

export function isAsyncFunction (object) {
  return object instanceof AsyncFunction
}

export function isArgumentsObject (object) {
  return isPlainObject(object)
}

export function isEmptyObject (object) {
  return (
    object !== null &&
    typeof object === 'object' &&
    Object.keys(object).length === 0
  )
}

export function isObject (object) {
  return (
    object !== null &&
    typeof object === 'object'
  )
}

export function isPlainObject (object) {
  return (
    object !== null &&
    typeof object === 'object' &&
    Object.getPrototypeOf(object) === Object.prototype
  )
}

export function isArrayBuffer (object) {
  return object !== null && object instanceof ArrayBuffer
}

export function isBufferLike (object) {
  return isArrayBuffer(object) || isTypedArray(object) || Buffer.isBuffer(object)
}

export function isFunction (value) {
  return typeof value === 'function' && !/^class/.test(value.toString())
}

export function isErrorLike (error) {
  if (error instanceof Error) return true
  return isObject(error) && 'name' in error && 'message' in error
}

export function isClass (value) {
  return (
    typeof value === 'function' &&
    value.prototype.constructor !== Function
  )
}

export function isPromiseLike (object) {
  return isFunction(object?.then)
}

export function toString (object) {
  return Object.prototype.toString(object)
}

export function toBuffer (object, encoding = undefined) {
  if (Buffer.isBuffer(object)) {
    return object
  } else if (isTypedArray(object)) {
    return Buffer.from(object.buffer)
  } else if (typeof object?.toBuffer === 'function') {
    return toBuffer(object.toBuffer(), encoding)
  }

  return Buffer.from(object, encoding)
}

export function toProperCase (string) {
  return string[0].toUpperCase() + string.slice(1)
}

export function splitBuffer (buffer, highWaterMark) {
  const buffers = []

  buffer = Buffer.from(buffer)

  do {
    buffers.push(buffer.slice(0, highWaterMark))
    buffer = buffer.slice(highWaterMark)
  } while (buffer.length > highWaterMark)

  if (buffer.length) {
    buffers.push(buffer)
  }

  return buffers
}

export function InvertedPromise () {
  const context = {}
  const promise = new Promise((resolve, reject) => {
    Object.assign(context, {
      resolve (value) {
        promise.value = value
        resolve(value)
        return promise
      },

      reject (error) {
        promise.error = error
        reject(error)
        return promise
      }
    })
  })

  return Object.assign(promise, context)
}

export function clamp (value, min, max) {
  if (!Number.isFinite(value)) {
    value = min
  }

  return Math.min(max, Math.max(min, value))
}

Object.defineProperties(promisify, {
  custom: {
    configurable: false,
    enumerable: false,
    value: Symbol.for('nodejs.util.promisify.custom')
  },
  args: {
    configurable: false,
    enumerable: false,
    value: Symbol.for('nodejs.util.promisify.args')
  }
})

export function promisify (original) {
  if (original && typeof original === 'object') {
    let object = Object.create(null)

    if (
      original[promisify.custom] &&
      typeof original[promisify.custom] === 'object'
    ) {
      object = original[promisify.custom]
    } else if (original.promises && typeof original.promises === 'object') {
      object = original.promises
    }

    for (const key in original) {
      object[key] = promisify(original[key])
    }

    Object.defineProperty(object, promisify.custom, {
      configurable: true,
      enumerable: false,
      writable: false,
      __proto__: null,
      value: object
    })

    return object
  }

  if (typeof original !== 'function') {
    throw new TypeError('Expecting original to be a function or object.')
  }

  if (original[promisify.custom]) {
    const fn = original[promisify.custom]
    Object.defineProperty(fn, promisify.custom, {
      configurable: true,
      enumerable: false,
      writable: false,
      __proto__: null,
      value: fn
    })

    return fn
  }

  const argumentNames = Array.isArray(original[promisify.args])
    ? original[promisify.args]
    : []

  async function fn (...args) {
    return await new Promise((resolve, reject) => {
      return Reflect.apply(original, this, args.concat(callback))
      function callback (err, ...values) {
        let [result] = values

        if (err) {
          return reject(err)
        }

        if (argumentNames.length) {
          result = {}

          for (let i = 0; i < argumentNames.length; ++i) {
            result[argumentNames[i]] = values[i]
          }
        }

        return resolve(result)
      }
    })
  }

  Object.setPrototypeOf(fn, Object.getPrototypeOf(original))

  return fn
}

export function inspect (value, options) {
  const ctx = {
    seen: options?.seen || [],
    depth: typeof options?.depth !== 'undefined' ? options.depth : 2,
    showHidden: options?.showHidden || false,
    customInspect: (
      options?.customInspect === undefined
        ? true
        : options.customInspect
    ),

    ...options,
    options
  }

  return formatValue(ctx, value, ctx.depth)

  function formatValue (ctx, value, depth) {
    // nodejs `value.inspect()` parity
    if (
      ctx.customInspect &&
      !(value?.constructor && value?.constructor?.prototype === value)
    ) {
      if (
        isFunction(value?.inspect) &&
        value?.inspect !== inspect &&
        value !== globalThis &&
        value !== globalThis?.system &&
        value !== globalThis?.__args &&
        value?.inspect[kIgnoreInspect] !== true
      ) {
        const formatted = value.inspect(depth, ctx)

        if (typeof formatted !== 'string') {
          return formatValue(ctx, formatted, depth)
        }

        return formatted
      } else if (
        (
          isFunction(value?.[kNodeCustomInspect]) &&
          value?.[kNodeCustomInspect] !== inspect
        ) ||
        (
          isFunction(value?.[kSocketCustomInspect]) &&
          value?.[kSocketCustomInspect] !== inspect
        )
      ) {
        const formatted = (value[kNodeCustomInspect] || value[kSocketCustomInspect]).call(
          value,
          depth,
          ctx,
          ctx.options,
          inspect
        )

        if (typeof formatted !== 'string') {
          return formatValue(ctx, formatted, depth)
        }

        return formatted
      }
    }

    if (value === undefined) {
      return 'undefined'
    }

    if (value === null) {
      return 'null'
    }

    if (typeof value === 'string') {
      const formatted = JSON.stringify(value)
        .replace(/^"|"$/g, '')
        .replace(/'/g, "\\'")
        .replace(/\\"/g, '"')

      return `'${formatted}'`
    }

    if (typeof value === 'number' || typeof value === 'boolean') {
      return String(value)
    }

    if (typeof value === 'bigint') {
      return String(value) + 'n'
    }

    if (value instanceof WeakSet) {
      return 'WeakSet { <items unknown> }'
    }

    if (value instanceof WeakMap) {
      return 'WeakMap { <items unknown> }'
    }

    let typename = ''

    const braces = ['{', '}']
    const isArrayLikeValue = isArrayLike(value)

    if (value instanceof Map) {
      braces[0] = `Map(${value.size}) ${braces[0]}`
    } else if (value instanceof Set) {
      braces[0] = `Set(${value.size}) ${braces[0]}`
    }

    const keys = value instanceof Map
      ? Array.from(value.keys())
      : new Set(Object.keys(value))

    const enumerableKeys = value instanceof Set
      ? Array(value.size).fill(0).map((_, i) => i)
      : Object.fromEntries(Array.from(keys).map((k) => [k, true]))

    if (ctx.showHidden) {
      try {
        const hidden = Object.getOwnPropertyNames(value)
        for (const key of hidden) {
          keys.add(key)
        }
      } catch (err) {}
    }

    if (isArrayLikeValue) {
      braces[0] = '['
      braces[1] = ']'
    }

    if (isAsyncFunction(value)) {
      const name = value.name ? `: ${value.name}` : ''
      typename = `[AsyncFunction${name}]`
    } else if (isFunction(value)) {
      const name = value.name ? `: ${value.name}` : ''
      typename = `[Function${name}]`
    }

    if (value instanceof RegExp) {
      typename = `${RegExp.prototype.toString.call(value)}`
    }

    if (value instanceof Date) {
      typename = `${Date.prototype.toString.call(value)}`
      braces[0] = '['
      braces[1] = ']'
    }

    if (value instanceof Error) {
      typename = `${Error.prototype.toString.call(value)}`
      braces[0] = ''
      braces[1] = ''

      if (value.cause) {
        keys.add('cause')
      }

      if (value.code) {
        enumerableKeys.code = true
        keys.add('code')
      }
    }

    if (!(value instanceof Map || value instanceof Set)) {
      if (
        typeof value === 'object' &&
        typeof value?.constructor === 'function' &&
        (value.constructor !== Object && value.constructor !== Array)
      ) {
        let tag = value?.[Symbol.toStringTag] || value.constructor.name || value?.toString

        if (typeof tag === 'function') {
          tag = tag.call(value)
        }

        if (tag === '[object Object]') {
          tag = ''
        }

        braces[0] = `${tag} ${braces[0]}`
      }

      if (keys.size === 0 && !(value instanceof Error)) {
        if (isFunction(value)) {
          return typename
        } else if (!isArrayLikeValue || value.length === 0) {
          return `${braces[0]}${typename}${braces[1]}`
        } else if (!isArrayLikeValue) {
          return typename
        }
      }
    }

    if (depth < 0) {
      if (value instanceof RegExp) {
        return RegExp.prototype.toString.call(value)
      }

      return '[Object]'
    }

    ctx.seen.push(value)

    const output = []

    if (isArrayLikeValue || value instanceof Set) {
      // const items = isArrayLikeValue ? value : Array.from(value.values())
      const size = isArrayLikeValue ? value.length : value.size
      for (let i = 0; i < size; ++i) {
        const key = String(i)
        if (value instanceof Set || hasOwnProperty(value, key)) {
          output.push(formatProperty(
            ctx,
            value,
            depth,
            enumerableKeys,
            key,
            true
          ))
        }
      }

      for (const key of keys) {
        if (!/^\d+$/.test(key)) {
          output.push(...Array.from(keys).map((key) => formatProperty(
            ctx,
            value,
            depth,
            enumerableKeys,
            key,
            true
          )))
        }
      }
    } else {
      output.push(...Array.from(keys).map((key) => formatProperty(
        ctx,
        value,
        depth,
        enumerableKeys,
        key,
        false
      )))
    }

    ctx.seen.pop()

    if (value instanceof Error) {
      let out = ''

      if (value?.message && !value?.stack?.startsWith(`${value?.name}: ${value?.message}`)) {
        out += `${value.name}: ${value.message}\n`
      }

      const formatWebkitErrorStackLine = (line) => {
        const [symbol = '', location = ''] = line.split('@')
        const output = []
        const root = new URL('../', import.meta.url).pathname

        let [context, lineno, colno] = (
          maybeURL(location)?.pathname?.split(/:/) ||
          location?.split(/:/) ||
          []
        )

        if (symbol) {
          output.push(symbol)
        }

        if (context) {
          context = context.replace(root, '')
          if (/socket\//.test(context)) {
            context = context.replace('socket/', 'socket:').replace(/.js$/, '')
          }
        }

        if (context && lineno && colno) {
          output.push(`(${context}:${lineno}:${colno})`)
        } else if (context && lineno) {
          output.push(`(${context}:${lineno})`)
        } else if (context) {
          output.push(`${context}`)
        }

        if (output.length) {
          output.unshift('    at')
        }
        return output.filter(Boolean).join(' ')
      }

      out += (value.stack || '')
        .split('\n')
        .map((line) => line.includes(`${value.name}: ${value.message}`) || /^\s*at\s/.test(line)
          ? line
          : formatWebkitErrorStackLine(line)
        )
        .filter(Boolean)
        .join('\n')

      if (keys.size) {
        out += ' {\n'
      }

      out += `  ${output.join(',\n  ')}`
      if (keys.size) {
        out += '\n}'
      }

      return out.trim()
    }

    const length = output.reduce((p, c) => (p + c.length + 1), 0)

    if (Object.getPrototypeOf(value) === null) {
      let tag = value?.[Symbol.toStringTag] || value?.toString

      if (typeof tag === 'function') {
        tag = tag.call(value)
      }

      braces[0] = `${tag || '[Object: null prototype]'} ${braces[0]}`
    }

    if (length > 80) {
      return `${braces[0]}\n${!typename ? '' : ` ${typename}\n`}  ${output.join(',\n  ')}\n${braces[1]}`
    }

    return `${braces[0]}${typename}${output.length ? ` ${output.join(', ')} ` : ''}${braces[1]}`
  }

  function formatProperty (
    ctx,
    value,
    depth,
    enumerableKeys,
    key,
    isArrayLikeValue
  ) {
    const descriptor = { value: undefined }
    const output = ['', '']

    try {
      descriptor.value = value[key]
    } catch (err) {}

    try {
      Object.assign(descriptor, Object.getOwnPropertyDescriptor(value, key))
    } catch (err) {}

    if (descriptor.get && descriptor.set) {
      output[1] = '[Getter/Setter]'
    } else if (descriptor.get) {
      output[1] = '[Getter]'
    } else if (descriptor.set) {
      output[1] = '[Setter]'
    }

    if (!hasOwnProperty(enumerableKeys, key)) {
      output[0] = `[${key}]`
    }

    if (!output[1]) {
      if (ctx.seen.includes(descriptor.value)) {
        output[1] = '[Circular]'
      } else {
        const tmp = value instanceof Set
          ? Array.from(value.values())[key]
          : value instanceof Map
            ? value.get(key)
            : descriptor.value

        if (depth === null) {
          output[1] = formatValue(ctx, tmp, null)
        } else {
          output[1] = formatValue(ctx, tmp, depth - 1)
        }

        if (output[1].includes('\n')) {
          if (isArrayLikeValue) {
            output[1] = output[1]
              .split('\n')
              .map((line) => `  ${line}`)
              .join('\n')
              .slice(2)
          } else {
            output[1] = '\n' + output[1]
              .split('\n')
              .map((line) => `    ${line}`)
              .join('\n')
          }
        }
      }
    }

    if (!output[0]) {
      if (isArrayLikeValue && /^\d+$/.test(key)) {
        return output[1]
      }

      output[0] = JSON.stringify(String(key))
        .replace(/^"/, '')
        .replace(/"$/, '')
        .replace(/'/g, "\\'")
        .replace(/\\"/g, '"')
        .replace(/(^"|"$)/g, "'")
    }

    if (value instanceof Map) {
      return output.join(' => ')
    } else {
      return output.join(': ')
    }
  }
}

export function format (format, ...args) {
  let options = args.pop()

  if (!options || typeof options !== 'object' || !options?.seen || !options?.depth) {
    if (options !== undefined) {
      args.push(options)
    }
    options = undefined
  }

  if (typeof format !== 'string') {
    return [format]
      .concat(args)
      .map((arg) => inspect(arg, { ...options }))
      .join(' ')
  }

  const regex = /%[dfijoOs%]/g

  let i = 0
  let str = format.replace(regex, (x) => {
    if (x === '%%') {
      return '%'
    }

    if (i >= args.length) {
      return x
    }

    if (args[i] === globalThis) {
      i++
    }

    if (args[i] === globalThis?.system) {
      i++
      return '[System]'
    }

    switch (x) {
      case '%d': return Number(args[i++])
      case '%f': return parseFloat(args[i++])
      case '%i': return parseInt(args[i++])
      case '%o': return inspect(args[i++], { showHidden: true })
      case '%O': return inspect(args[i++])
      case '%j':
        try {
          return JSON.stringify(args[i++])
        } catch (_) {
          return '[Circular]'
        }

      case '%s': return String(args[i++])
    }

    return x
  })

  for (const arg of args.slice(i)) {
    if (arg === null || typeof arg !== 'object') {
      str += ' ' + arg
    } else {
      str += ' ' + inspect(arg, { ...options })
    }
  }

  return str
}

export function parseJSON (string) {
  if (string !== null) {
    try {
      return JSON.parse(String(string))
    } catch (err) {}
  }

  return null
}

export function parseHeaders (headers) {
  if (typeof headers !== 'string') {
    return []
  }

  return headers
    .split('\n')
    .map((l) => l.trim().split(':'))
    .filter((e) => e.length === 2)
    .map((e) => [e[0].trim().toLowerCase(), e[1].trim().toLowerCase()])
}

export function noop () {}

export class IllegalConstructor {
  constructor () {
    throw new IllegalConstructorError()
  }
}

const percentageRegex = /^(100(\.0+)?|[1-9]?\d(\.\d+)?)%$/

export function isValidPercentageValue (input) {
  return percentageRegex.test(input)
}

export function compareBuffers (a, b) {
  return toBuffer(a).compare(toBuffer(b))
}

export default exports
