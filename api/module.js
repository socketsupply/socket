/**
 * @module Module
 */
import { createRequire, Module } from './commonjs/module.js'
import builtins, { isBuiltin } from './commonjs/builtins.js'

export { createRequire, Module, builtins, isBuiltin }

export const builtinModules = builtins

export default Module
