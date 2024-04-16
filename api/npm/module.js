import { DEFAULT_PACKAGE_PREFIX, Package } from '../commonjs/package.js'
import { Loader } from '../commonjs/loader.js'
import location from '../location.js'
import path from '../path.js'

/**
 * @typedef {{
 * package: Package
 * origin: string,
 * type: 'commonjs' | 'module',
 * url: string
 * }} ModuleResolution
 */

/**
 * Resolves an NPM module for a given `specifier` and an optional `origin`.
 * @param {string|URL} specifier
 * @param {string|URL=} [origin]
 * @param {{ prefix?: string, type?: 'commonjs' | 'module' }} [options]
 * @return {ModuleResolution|null}
 */
export async function resolve (specifier, origin = null, options = null) {
  if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
    options = origin
    origin = options.origin ?? null
  }

  if (!origin) {
    origin = location.origin
  }

  if (!origin.endsWith('/')) {
    origin += '/'
  }

  // just use `specifier` to derive name and origin if it was a URL
  if (specifier instanceof URL) {
    origin = specifier.origin
    specifier = specifier.pathname.slice(1) + specifier.search
  }

  const prefix = options?.prefix ?? DEFAULT_PACKAGE_PREFIX
  const name = Package.Name.from(specifier, { origin })
  const type = options?.type ?? 'module' // prefer 'module' in this context
  const pkg = new Package(name.value, {
    loader: { origin }
  })

  const pathname = name.pathname.replace(name.value, '.') || '.'

  try {
    // will call `pkg.load()` internally
    // can throw `ModuleNotFoundError`
    const url = pkg.resolve(pathname, { prefix, type })
    return {
      package: pkg,
      origin: pkg.origin,
      type: pkg.type,
      url
    }
  } catch (err) {
    if (err?.code === 'MODULE_NOT_FOUND') {
      const url = new URL(pathname, new URL(prefix + name.value + '/', origin))
      const loader = new Loader({ extensions: [path.extname(url.href)] })
      const status = loader.status(url)
      // check for regular file
      if (status.ok) {
        return {
          package: pkg,
          origin: origin + prefix,
          type: 'file',
          url: url.href
        }
      }

      return null
    }

    throw err
  }
}

export default {
  resolve
}
