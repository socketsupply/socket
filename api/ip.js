/**
 * @module IP
 *
 * Various functions for working with IP addresses.
 *
 * Example usage:
 * ```js
 * import ip from 'socket:ip'
 *
 * console.log(ip.normalizeIPv4('127')) // '0.0.0.127'
 * console.log(ip.normalizeIPv4('1.128')) // '0.0.1.128'
 * console.log(ip.normalizeIPv4([0, 128])) // '0.0.0.128'
 * console.log(ip.normalizeIPv4(Uint8Array.from([127, 0, 0, 01])) // '127.0.0.1'
 *
 * console.log(ip.isIPv4('127')) // true
 * console.log(ip.isIPv4('1.128')) // true
 * console.log(ip.isIPv4([0, 128])) // true
 * console.log(ip.isIPv4(Uint8Array.from([127, 0, 0, 01])) true
 *
 * console.log(ip.isIPv4('x')) // false
 * console.log(ip.isIPv4('1.x')) // false
 * console.log(ip.isIPv4([0, 1, 2, 3, 4]))) // false
 * console.log(ip.isIPv4([-1])) // false
 * console.log(ip.isIPv4(Uint8Array.from([127, 0, 0, 01])) false
 * ```
 */

const ipv4SegmentPattern = '(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])'
const ipv4StringPattern = `(${ipv4SegmentPattern}[.]){3}${ipv4SegmentPattern}`
const iPv4Regex = new RegExp(`^${ipv4StringPattern}$`)

/**
 * Normalizes an IPv4 address string.
 * @ignore
 * @param {string} string
 * @return {string}
 */
function normalizeIPv4AddressString (string) {
  if (!string || typeof string !== 'string') {
    return ''
  }

  const parts = string.split('.')
  while (parts.length < 4) {
    parts.unshift('0')
  }

  return parts
    .map((part) => parseInt(part))
    .filter((value) => value >= 0 && value <= 255)
    .join('.')
}

/**
 * Normalizes input as an IPv4 address string
 * @param {string|object|string[]|Uint8Array} input
 * @return {string}
 */
export function normalizeIPv4 (input) {
  if (Array.isArray(input)) {
    return normalizeIPv4(input.join('.'))
  } else if (typeof input === 'object' && typeof input?.byteLength === 'number') {
    return normalizeIPv4(Array.from(input))
  } else if (typeof input === 'object' && typeof input?.length === 'number') {
    const array = []
    for (let i = 0; i < input.length; ++i) {
      array.push(input[i])
    }
    return normalizeIPv4(array)
  }

  return normalizeIPv4AddressString(String(input))
}

/**
 * Determines if an input `string` is in IP address version 4 format.
 * @param {string|object|string[]|Uint8Array} input
 * @return {boolean}
 */
export function isIPv4 (input) {
  return iPv4Regex.test(normalizeIPv4(input))
}

export default {
  normalizeIPv4,
  isIPv4
}
