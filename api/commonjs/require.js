import { DEFAULT_PACKAGE_PREFIX, Package } from './package.js'
import { getBuiltin, isBuiltin } from './builtins.js'
import { ModuleNotFoundError } from '../errors.js'
import { Loader } from './loader.js'
import location from '../location.js'

/**
 * @typedef {function(string, import('./module.js').Module, function(string): any): any} RequireResolver
 */

/**
 * @typedef {{
 *   module: import('./module.js').Module,
 *   prefix?: string,
 *   request?: import('./loader.js').RequestOptions,
 *   builtins?: object
 * }} CreateRequireOptions
 */

/**
 * @typedef {function(string): any} RequireFunction
 */

/**
 * @typedef {import('./package.js').PackageOptions} PackageOptions
 */

/**
 * @typedef {import('./package.js').PackageResolveOptions} PackageResolveOptions
 */

/**
 * @typedef {PackageResolveOptions & PackageOptions} ResolveOptions
 */

/**
 * @typedef {ResolveOptions & {
 *   resolvers?: RequireResolver[],
 *   importmap?: import('./module.js').ImportMap,
 * }} RequireOptions
 */

/**
 * An array of global require paths, relative to the origin.
 * @type {string[]}
 */
export const globalPaths = [
  new URL(DEFAULT_PACKAGE_PREFIX, location.origin).href
]

/**
 * Factory for creating a `require()` function based on a module context.
 * @param {CreateRequireOptions} options
 * @return {RequireFunction}
 */
export function createRequire (options) {
  const { builtins, module } = options
  const { cache, main } = module

  Object.assign(resolve, {
    paths
  })

  return Object.assign(require, {
    resolve,
    cache,
    main
  })

  /**
   * Requires a module at for a given `input` which can be a relative file,
   * named module, or an absolute URL.
   * @param {string|URL} athname
   * @param {RequireOptions=} [options]
   * @return {string}
   */
  function require (input, options = null) {
    if (isBuiltin(input, { builtins: options?.builtins ?? builtins })) {
      return getBuiltin(input, { builtins: options?.builtins ?? builtins })
    }

    const resolved = resolve(input, {
      type: module.package.type,
      ...options
    })

    if (cache[input]) {
      return cache[input].exports
    }

    if (!resolved) {
      throw new ModuleNotFoundError(
        `Cannnot find module '${input}`,
        module.children.map((mod) => mod.id)
      )
    }

    const child = module.createModule(resolved, {
      ...options,
      package: input.startsWith('.') || input.startsWith('/')
        ? module.package
        : Package.find(resolved, {
          loader: new Loader(module.loader),
          ...options
        })
    })

    if (child.load(options)) {
      cache[resolved] = child
      return child.exports
    }
  }

  /**
   * Resolve a module `input` to an absolute URL.
   * @param {string|URL} pathname
   * @param {ResolveOptions=} [options]
   * @return {string}
   */
  function resolve (input, options = null) {
    if (isBuiltin(input, builtins)) {
      return input
    }

    const url = URL.canParse(input)
      ? new URL(input)
      : null

    const origins = url !== null
      ? [url.origin]
      : resolve.paths(input)

    if (url !== null) {
      input = `.${url.pathname + url.search}`
    }

    for (const origin of origins) {
      if (url) {
        const status = module.loader.status(input, origin, options)

        if (status.ok) {
          return status.id
        }
      } else if (input.startsWith('.') || input.startsWith('/')) {
        return module.resolve(input)
      } else {
        const components = input.split('/')
        const manifest = new Package(components.shift(), {
          prefix: '',
          loader: new Loader(origin, module.loader)
        })

        const loaded = manifest.load({
          type: module.package.type,
          ...options
        })

        if (loaded) {
          const pathname = components.join('/') || '.'
          return manifest.resolve(pathname, {
            type: module.package.type,
            ...options
          })
        }
      }
    }
  }

  /**
   * Computes possible `require()` origin paths for an input module URL
   * @param {string|URL} pathname
   * @return {string[]?}
   */
  function paths (input) {
    if (isBuiltin(input, builtins)) {
      return null
    }

    if (URL.canParse(input)) {
      return [new URL(input).origin]
    }

    if (input.startsWith('.') || input.startsWith('/')) {
      return [module.origin]
    }

    const origins = new Set(globalPaths.map((path) => new URL(path, location.origin).href))
    let origin = module.origin

    while (true) {
      const url = new URL(origin)
      origins.add(origin)

      if (url.pathname === '/') {
        break
      }

      origin = new URL('..', origin).href
    }

    return Array
      .from(origins)
      .map((origin) => new URL(options.prefix, origin))
      .map((url) => url.href)
  }
}

export default createRequire
