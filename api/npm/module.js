import { Module, createRequire } from '../module.js'
import { MIMEType } from '../mime.js'
import location from '../location.js'
import path from '../path.js'

export async function resolve (specifier, origin = null) {
  if (!origin) {
    origin = location.origin
  }

  const parts = specifier.split('/')
  const prefix = './node_modules'
  const pathname = parts.slice(1).join('/').trim()
  const moduleName = parts[0]
  let packageJSON = null
  let resolved = null

  const require = createRequire(origin, {
    headers: {
      'Runtime-ServiceWorker-Fetch-Mode': 'ignore'
    }
  })

  try {
    packageJSON = require(`${prefix}/${moduleName}/package.json`)
  } catch {
    try {
      require(`${prefix}/${moduleName}/index.js`)
      packageJSON = {
        type: 'commonjs',
        main: `${prefix}/${moduleName}/index.js`
      }
    } catch (err) {
      if (
        (err.name === 'SyntaxError' && err.message.includes('import call')) ||
        (err.name === 'SyntaxError' && /unexpected keyword '(export|import|default|from)'/i.test(err.message)) ||
        (err.message.includes('import.meta'))
      ) {
        return {
          origin,
          type: 'module',
          path: `${prefix}/${moduleName}/index.js`
        }
      } else {
        throw err
      }
    }
  }

  // top level import
  if (pathname.length === 0) {
    // legacy
    if (packageJSON.module) {
      resolved = {
        type: 'module',
        path: path.join(`${prefix}/${moduleName}`, packageJSON.module)
      }
    }

    // simple
    if (typeof packageJSON.exports === 'string') {
      resolved = {
        type: packageJSON.type,
        path: path.join(`${prefix}/${moduleName}`, packageJSON.exports)
      }
    }

    // exports object
    if (packageJSON.exports && typeof packageJSON.exports === 'object') {
      if (typeof packageJSON.exports.import === 'string') {
        resolved = {
          type: 'module',
          path: path.join(`${prefix}/${moduleName}`, packageJSON.exports.import)
        }
      } else if (typeof packageJSON.exports.require === 'string') {
        resolved = {
          type: 'commonjs',
          path: path.join(`${prefix}/${moduleName}`, packageJSON.exports.require)
        }
      } else if (typeof packageJSON.exports['.'] === 'string') {
        resolved = {
          type: packageJSON.type,
          path: path.join(`${prefix}/${moduleName}`, packageJSON.exports['.'])
        }
      } else if (typeof packageJSON.exports['.']?.import === 'string') {
        resolved = {
          type: 'module',
          path: path.join(`${prefix}/${moduleName}`, packageJSON.exports['.'].import)
        }
      } else if (typeof packageJSON.exports['.']?.require === 'string') {
        resolved = {
          type: 'commonjs',
          path: path.join(`${prefix}/${moduleName}`, packageJSON.exports['.'].require)
        }
      }
    } else if (typeof packageJSON.main === 'string') {
      resolved = {
        type: 'commonjs',
        path: path.join(`${prefix}/${moduleName}`, packageJSON.main)
      }
    } else {
      resolved = {
        type: packageJSON.type ?? 'commonjs',
        path: path.join(`${prefix}/${moduleName}`, 'index.js')
      }
    }
  }

  if (!resolved && pathname.length) {
    // no exports, just join paths
    if (!packageJSON.exports || typeof packageJSON.exports === 'string') {
      resolved = {
        type: packageJSON.type ?? 'commonjs',
        path: path.join(`${prefix}/${moduleName}`, pathname)
      }
    } else if (packageJSON.exports && typeof packageJSON.exports === 'object') {
      const extensions = [
        '',
        '.js',
        '.mjs',
        '.cjs',
        '.json'
      ]

      do {
        const name = `./${pathname}` + extensions.shift()
        if (typeof packageJSON.exports[name] === 'string') {
          resolved = {
            type: packageJSON.type ?? 'commonjs',
            path: path.join(`${prefix}/${moduleName}`, packageJSON.exports[name])
          }
          break
        } else if (packageJSON.exports[name] && typeof packageJSON.exports[name] === 'object') {
          const exports = packageJSON.exports[name]
          if (packageJSON.type === 'module' && exports.import) {
            resolved = {
              type: 'module',
              path: path.join(`${prefix}/${moduleName}`, exports.import)
            }
            break
          } else if (exports.require) {
            resolved = {
              type: 'commonjs',
              path: path.join(`${prefix}/${moduleName}`, exports.require)
            }

            break
          }
        }
      } while (extensions.length)
    }
  }

  if (!resolved) {
    const filename = path.join(`${prefix}/${moduleName}`, pathname)
    try {
      const module = Module.from(filename, Module.main)
      const loaded = module.load({
        evaluate: false,
        headers: {
          'Runtime-ServiceWorker-Fetch-Mode': 'ignore'
        }
      })

      if (loaded) {
        resolved = {
          type: 'commonjs',
          path: new URL(module.id).pathname
        }
      }
    } catch {}
  }

  if (!resolved) {
    return null
  }

  if (resolved.path.endsWith('/')) {
    resolved.path += 'index.js'
  }

  if (!path.extname(resolved.path)) {
    const extensions = [
      '.js',
      '.mjs',
      '.cjs',
      '.json',
      '' // maybe an extension was omitted for the actual file, try that _last_
    ]

    do {
      resolved.path = resolved.path + extensions.shift()

      try {
        const response = await fetch(new URL(resolved.path, origin))

        if (response.ok) {
          const { essence } = new MIMEType(response.headers.get('content-type'))
          const contentLength = parseInt(response.headers.get('content-length'))
          if (contentLength > 0) {
            if (essence === 'text/javascript' || essence === 'application/javascript') {
              break
            }
          }
        }
      } catch {
        // ignore
      }

      if (extensions.length === 0) {
        return null
      }
    } while (extensions.length)
  }

  resolved.origin = origin
  return resolved
}

export default {
  resolve
}
