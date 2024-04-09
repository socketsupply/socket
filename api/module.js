/**
 * @module Module
 */
import { createRequire, Module } from './commonjs/module.js'
import builtins, { isBuiltin } from './commonjs/builtins.js'

/**
 * @typedef {import('./commonjs/module.js').ModuleOptions} ModuleOptions
 * @typedef {import('./commonjs/module.js').ModuleResolver} ModuleResolver
 * @typedef {import('./commonjs/module.js').ModuleLoadOptions} ModuleLoadOptions
 * @typedef {import('./commonjs/module.js').RequireFunction} RequireFunction
 * @typedef {import('./commonjs/module.js').CreateRequireOptions} CreateRequireOptions
 */

export { createRequire, Module, builtins, isBuiltin }
export const builtinModules = builtins

export default Module
