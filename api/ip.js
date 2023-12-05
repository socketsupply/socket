/**
 * @module IP
 * Functions for working with IP addresses
 *
 */

const v4Seg = '(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])'
const v4Str = `(${v4Seg}[.]){3}${v4Seg}`
const IPv4Reg = new RegExp(`^${v4Str}$`)

export const isIPv4 = s => {
  return IPv4Reg.test(s)
}
