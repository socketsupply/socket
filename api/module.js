/**
 * @module Module
 */
import builtins, { isBuiltin } from './commonjs/builtins.js'
import { createRequire } from './commonjs/require.js'
import { Module } from './commonjs/module.js'

export {
  Module,
  builtins,
  isBuiltin,
  createRequire
}

export const builtinModules = builtins

export default Module
