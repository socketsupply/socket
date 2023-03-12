import { Path } from './path.js'
import posix from './posix.js'

/** @typedef {import('./path.js').PathComponent} PathComponent */

export { posix }
export * as default from './win32.js'
export * as win32 from './win32.js'

export const sep = '\\'
export const delimiter = ';'

/**
 * Computes current working directory for a path
 * @param {string}
 */
export function cwd () {
  return Path.cwd().replace(/\//g, '\\')
}

/**
 * Resolves path components to an absolute path.
 * @param {...PathComponent} components
 * @return {string}
 */
export function resolve (...components) {
  return Path.resolve({ sep }, cwd(), ...components).replace(/\//g, '\\')
}

/**
 * Joins path components. This function may not return an absolute path.
 * @param {...PathComponent} components
 * @return {string}
 */
export function join (...components) {
  return Path.join({ sep }, ...components).replace(/\//g, '\\')
}

/**
 * Computes directory name of path.
 * @param {PathComponent} path
 * @return {string}
 */
export function dirname (path) {
  return Path.dirname({ sep }, path).replace(/\//g, '\\')
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
  return Path.from(path)
}

/**
 * Formats `Path` object into a string.
 * @param {object|Path} path
 * @return {string}
 */
export function format (path) {
  return Path.format({ sep: path.sep || sep }, path).replace(/\//g, '\\')
}

/**
 * Normalizes `path` resolving `..` and `.\` preserving trailing
 * slashes.
 * @param {string} path
 */
export function normalize (path) {
  return Path.normalize({ sep }, path).replace(/\//g, '\\')
}

/**
 * Computes the relative path from `from` to `to`.
 * @param {string} from
 * @param {string} to
 * @return {string}
 */
export function relative (from, to) {
  return Path.relative({ sep }, from, to).replace(/\//g, '\\')
}
