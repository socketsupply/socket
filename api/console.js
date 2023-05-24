import { format, isObject } from './util.js'
import { postMessage } from './ipc.js'

function isPatched (console) {
  return console?.[Symbol.for('socket.console.patched')] === true
}

function table (data, columns, formatValues = true) {
  const maybeFormat = (value) => formatValues ? format(value) : value
  const rows = []

  columns = Array.isArray(columns) && Array.from(columns)

  if (!columns) {
    columns = []

    if (Array.isArray(data)) {
      const first = data[0]

      if (!isObject(first) && first !== undefined) {
        columns.push('Values')
      } else if (isObject(first)) {
        const keys = new Set(data
          .map(Object.keys)
          .reduce((a, b) => a.concat(b), [])
        )

        columns.push(...keys.values())
      }
    } else if (isObject(data)) {
      columns.push('Value')

      for (const key in data) {
        const value = data[key]
        if (isObject(value)) {
          columns.push(key)
        }
      }
    }
  }

  if (Array.isArray(data)) {
    const first = data[0]

    if (!isObject(first) && first !== undefined) {
      for (let i = 0; i < data.length; ++i) {
        const value = maybeFormat(data[i] ?? null)
        rows.push([i, value])
      }
    } else {
      for (let i = 0; i < data.length; ++i) {
        const row = [i]
        for (const column of columns) {
          if (column === 'Value' && !isObject(data[i])) {
            const value = maybeFormat(data[i] ?? null)
            row.push(value ?? null)
          } else {
            const value = maybeFormat(data[i]?.[column] ?? data[i] ?? null)
            row.push(value)
          }
        }

        rows.push(row)
      }
    }
  } else if (isObject(data)) {
    for (const key in data) {
      rows.push([key, maybeFormat(data[key] ?? null)])
    }
  }

  columns.unshift('(index)')

  return { columns, rows }
}

export const globalConsole = globalThis?.console ?? null

export class Console {
  /**
   * @type {import('dom').Console}
   */
  console = null

  /**
   * @type {Map}
   */
  timers = new Map()

  /**
   * @type {Map}
   */
  counters = new Map()

  /**
   * @type {function?}
   */
  postMessage = null

  /**
   * @ignore
   */
  constructor (options) {
    if (typeof options?.postMessage !== 'function') {
      throw new TypeError('Expecting `.postMessage` in constructor options')
    }

    Object.defineProperties(this, {
      postMessage: {
        ...Object.getOwnPropertyDescriptor(this, 'postMessage'),
        enumerable: false,
        configurable: false,
        value: options?.postMessage
      },

      console: {
        ...Object.getOwnPropertyDescriptor(this, 'console'),
        enumerable: false,
        configurable: false,
        value: options?.console ?? null
      },

      counters: {
        ...Object.getOwnPropertyDescriptor(this, 'counters'),
        enumerable: false,
        configurable: false
      },

      timers: {
        ...Object.getOwnPropertyDescriptor(this, 'timers'),
        enumerable: false,
        configurable: false
      }
    })
  }

  async write (destination, ...args) {
    const value = encodeURIComponent(format(...args))
    const uri = `ipc://${destination}?value=${value}`
    try {
      return await this.postMessage?.(uri)
    } catch (err) {
      this.console?.warn?.(`Failed to write to ${destination}: ${err.message}`)
    }
  }

  assert (assertion, ...args) {
    this.console?.assert?.(assertion, ...args)
    if (Boolean(assertion) !== true) {
      this.write('stderr', `Assertion failed: ${format(...args)}`)
    }
  }

  clear () {
    this.console?.clear?.()
  }

  count (label = 'default') {
    this.console?.count(label)
    if (!isPatched(this.console)) {
      const count = (this.counters.get(label) || 0) + 1
      this.counters.set(label, count)
      this.write('stdout', `${label}: ${count}`)
    }
  }

  countReset (label = 'default') {
    this.console?.countReset()
    if (!isPatched(this.console)) {
      this.counters.set(label, 0)
      this.write('stdout', `${label}: 0`)
    }
  }

  debug (...args) {
    this.console?.debug?.(...args)
    if (!isPatched(this.console)) {
      this.write('stderr', ...args)
    }
  }

  dir (...args) {
    this.console?.dir?.(...args)
  }

  dirxml (...args) {
    this.console?.dirxml?.(...args)
  }

  error (...args) {
    this.console?.error?.(...args)
    if (!isPatched(this.console)) {
      this.write('stderr', ...args)
    }
  }

  info (...args) {
    this.console?.info?.(...args)
    if (!isPatched(this.console)) {
      this.write('stdout', ...args)
    }
  }

  log (...args) {
    this.console?.log?.(...args)
    if (!isPatched(this.console)) {
      this.write('stdout', ...args)
    }
  }

  table (...args) {
    if (isPatched(this.console)) {
      return this.console.table(...args)
    }

    if (!isObject(args[0])) {
      return this.log(...args)
    }

    // @ts-ignore
    const { columns, rows } = table(...args)
    const output = []
    const widths = []

    for (let i = 0; i < columns.length; ++i) {
      const column = columns[i]

      let columnWidth = column.length + 2
      for (const row of rows) {
        const cell = row[i]
        const cellContents = String(cell)
        const cellWidth = 2 + (cellContents
          .split('\n')
          .map((r) => r.length)
          .sort()
          .slice(-1)[0] ?? 0
        )

        columnWidth = Math.max(columnWidth, cellWidth)
      }

      columnWidth += 2
      widths.push(columnWidth)
      output.push(column.padEnd(columnWidth, ' '))
    }

    output.push('\n')

    for (let i = 0; i < rows.length; ++i) {
      const row = rows[i]
      for (let j = 0; j < row.length; ++j) {
        const width = widths[j]
        const cell = String(row[j])
        output.push(cell.padEnd(width, ' '))
      }

      output.push('\n')
    }

    this.write('stdout', output.join(''))
    this.console?.table?.(...args)
  }

  time (label = 'default') {
    this.console?.time?.(label)
    if (!isPatched(this.console)) {
      if (this.timers.has(label)) {
        this.console?.warn?.(
          `Warning: Label '${label}' already exists for console.time()`
        )
      } else {
        this.timers.set(label, Date.now())
      }
    }
  }

  timeEnd (label = 'default') {
    this.console?.timeEnd?.(label)
    if (!isPatched(this.console)) {
      if (!this.timers.has(label)) {
        this.console?.warn?.(
          `Warning: No such label '${label}' for console.timeEnd()`
        )
      } else {
        const time = this.timers.get(label)
        this.timers.delete(label)
        if (typeof time === 'number' && time >= 0) {
          const elapsed = Date.now() - time

          if (elapsed >= 1000) {
            this.write('stdout', `${label}: ${elapsed * 0.001}s`)
          } else {
            this.write('stdout', `${label}: ${elapsed}ms`)
          }
        }
      }
    }
  }

  timeLog (label = 'default') {
    this.console?.timeLog?.(label)
    if (!isPatched(this.console)) {
      if (!this.timers.has(label)) {
        this.console?.warn?.(
          `Warning: No such label '${label}' for console.timeLog()`
        )
      } else {
        const time = this.timers.get(label)
        if (typeof time === 'number' && time >= 0) {
          const elapsed = Date.now() - time

          if (elapsed * 0.001 > 0) {
            this.write('stdout', `${label}: ${elapsed * 0.001}s`)
          } else {
            this.write('stdout', `${label}: ${elapsed}ms`)
          }
        }
      }
    }
  }

  trace (...objects) {
    this.console?.trace?.(...objects)
    if (!isPatched(this.console)) {
      const stack = new Error().stack.split('\n').slice(1)
      stack.unshift(`Trace: ${format(...objects)}`)
      this.write('stderr', stack.join('\n'))
    }
  }

  warn (...args) {
    this.console?.warn?.(...args)
    if (!isPatched(this.console)) {
      this.write('stderr', ...args)
    }
  }
}

export function patchGlobalConsole (globalConsole, options = {}) {
  if (!globalConsole || typeof globalConsole !== 'object') {
    globalConsole = globalThis?.console
  }

  if (!globalConsole) {
    throw new TypeError('Cannot determine a global console object to patch')
  }

  if (!isPatched(globalConsole)) {
    const defaultConsole = new Console({
      postMessage,
      ...options
    })

    globalConsole[Symbol.for('socket.console.patched')] = true

    for (const key in globalConsole) {
      if (typeof Console.prototype[key] === 'function') {
        const original = globalConsole[key].bind(globalConsole)
        globalConsole[key] = (...args) => {
          original(...args)
          defaultConsole[key](...args)
        }
      }
    }
  }

  return globalConsole
}

export default new Console({
  postMessage,
  console: patchGlobalConsole(globalConsole)
})
