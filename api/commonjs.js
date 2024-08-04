/**
 * @module commonjs
 */
import builtins, { defineBuiltin } from './commonjs/builtins.js'
import createRequire from './commonjs/require.js'
import Package from './commonjs/package.js'
import Module from './commonjs/module.js'
import Loader from './commonjs/loader.js'
import Cache from './commonjs/cache.js'

import * as exports from './commonjs.js'

export default exports
export {
  builtins,
  Cache,
  createRequire,
  Loader,
  Module,
  Package
}

defineBuiltin('commonjs', exports)
