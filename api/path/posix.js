import * as win32 from './win32.js'
import location from '../location.js'
import { Path } from './path.js'
import url from '../url.js'

import {
  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC
} from './well-known.js'

import * as exports from './posix.js'

/** @typedef {import('./path.js').PathComponent} PathComponent */

export {
  win32,
  Path,

  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC
}

export default exports

export const posix = exports
export const sep = '/'
export const delimiter = ':'

/**
 * Computes current working directory for a path
 * @param {string}
 * @return {string}
 */
export function cwd () {
  return url.resolve(location.pathname, '.')
}

/**
 * Resolves path components to an absolute path.
 * @param {...PathComponent} components
 * @return {string}
 */
export function resolve (...components) {
  return Path.resolve({ sep }, ...components)
}

/**
 * Joins path components. This function may not return an absolute path.
 * @param {...PathComponent} components
 * @return {string}
 */
export function join (...components) {
  return Path.join({ sep }, ...components)
}

/**
 * Computes directory name of path.
 * @param {PathComponent} path
 * @return {string}
 */
export function dirname (path) {
  return Path.dirname({ sep }, path)
}

/**
 * Computes base name of path.
 * @param {PathComponent} path
 * @param {string} suffix
 * @return {string}
 */
export function basename (path, suffix) {
  return Path.basename({ sep }, path).replace(suffix || '', '')
}

/**
 * Computes extension name of path.
 * @param {PathComponent} path
 * @return {string}
 */
export function extname (path) {
  return Path.extname({ sep }, path)
}

/**
 * Predicate helper to determine if path is absolute.
 * @param {PathComponent} path
 * @return {boolean}
 */
export function isAbsolute (path) {
  return Path.from(path, '.').isRelative === false
}

/**
 * Parses input `path` into a `Path` instance.
 * @param {PathComponent} path
 * @return {Path}
 */
export function parse (path) {
  return Path.parse(path)
}

/**
 * Formats `Path` object into a string.
 * @param {object|Path} path
 * @return {string}
 */
export function format (path) {
  return Path.format({ sep: path.sep || sep }, path)
}

/**
 * Normalizes `path` resolving `..` and `./` preserving trailing
 * slashes.
 * @param {string} path
 */
export function normalize (path) {
  return Path.normalize({ sep }, path)
}

/**
 * Computes the relative path from `from` to `to`.
 * @param {string} from
 * @param {string} to
 * @return {string}
 */
export function relative (from, to) {
  return Path.relative({ sep }, from, to)
}
