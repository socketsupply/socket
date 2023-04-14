/**
 * @module OS
 *
 * This module provides normalized system information from all the major
 * operating systems.
 *
 * Example usage:
 * ```js
 * import { arch, platform } from 'socket:os'
 * ```
 */

import { toProperCase } from './util.js'
import ipc, { primordials } from './ipc.js'

const UNKNOWN = 'unknown'

// eslint-disable-next-line new-parens
const cache = new class {
  type = UNKNOWN

  /**
   * @type {boolean}
   * @ignore
   */
  isWindows = false

  /**
   * @type {object?}
   * @ignore
   */
  networkInterfaces
  /**
   * @type {number}
   * @ignore
   */
  networkInterfacesTTL = 0

  /**
   * @type {object?}
   * @ignore
   */
  cpus

  /**
   * @type {string?}
   * @ignore
   */
  uname
}

export function arch () {
  return primordials.arch
}

export function cpus () {
  if (!cache.cpus) {
    const { err, data } = ipc.sendSync('os.cpus')
    if (err) throw err
    cache.cpus = data
  }

  return cache.cpus
}

export function networkInterfaces () {
  const now = Date.now()

  if (cache.networkInterfaces && cache.networkInterfacesTTL > now) {
    return cache.networkInterfaces
  }

  const interfaces = {}

  const result = ipc.sendSync('os.networkInterfaces')
  if (!result.data) return interfaces

  const { ipv4, ipv6 } = result.data

  for (const type in ipv4) {
    const info = typeof ipv4[type] === 'string'
      ? { address: ipv4[type] }
      : ipv4[type]

    const { address } = info
    const family = 'IPv4'

    let internal = info.internal || false
    let netmask = info.netmask || '255.255.255.0'
    let cidr = `${address}/24`
    let mac = info.mac || null

    if (address === '127.0.0.1') {
      internal = true
      mac = '00:00:00:00:00:00'
      cidr = '127.0.0.1/8'
      netmask = '255.0.0.0'
    }

    interfaces[type] = interfaces[type] || []
    interfaces[type].push({
      address,
      netmask,
      internal,
      family,
      cidr,
      mac
    })
  }

  for (const type in ipv6) {
    const info = typeof ipv6[type] === 'string'
      ? { address: ipv6[type] }
      : ipv6[type]

    const { address } = info
    const family = 'IPv6'

    let internal = info.internal || false
    let netmask = internal.netmask || 'ffff:ffff:ffff:ffff::'
    let cidr = `${address}/64`
    let mac = info.mac || null

    if (address === '::1') {
      internal = true
      netmask = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff'
      cidr = '::1/128'
      mac = '00:00:00:00:00:00'
    }

    interfaces[type] = interfaces[type] || []
    interfaces[type].push({
      address,
      netmask,
      internal,
      family,
      cidr,
      mac
    })
  }

  cache.networkInterfaces = interfaces
  cache.networkInterfacesTTL = Date.now() + 512

  return interfaces
}

export function platform () {
  return primordials.platform
}

export function type () {
  let value = 'unknown'

  if (cache.type !== UNKNOWN) {
    return cache.type
  }

  if (globalThis !== globalThis.window) {
    switch (platform()) {
      case 'android': return 'Linux'
      case 'cygwin': return 'CYGWIN_NT'
      case 'freebsd': return 'FreeBSD'
      case 'linux': return 'Linux'
      case 'openbsd': return 'OpenBSD'
      case 'win32': return 'Windows_NT'

      case 'ios': case 'mac': case 'Mac': case 'darwin': return 'Darwin'
    }
  }

  if (globalThis === globalThis.window) {
    value = primordials.platform
  }

  value = value.replace(/android/i, 'Linux')
  value = value.replace(/ios/i, 'Darwin')

  if (value !== UNKNOWN) {
    value = toProperCase(value)
  }

  cache.type = value

  return cache.type
}

// TODO: non-standard function. Do we need it?
export function isWindows () {
  if (cache.isWindows) {
    return cache.isWindows
  }

  cache.isWindows = /^win.*/i.test(type())
  return cache.isWindows
}

export function tmpdir () {
  let path = ''

  if (isWindows()) {
    path = (
      globalThis.__args.env.TEMPDIR ||
      globalThis.__args.env.TMPDIR ||
      globalThis.__args.env.TEMP ||
      globalThis.__args.env.TMP ||
      (globalThis.__args.env.SystemRoot ?? globalThis.__args.env.windir ?? '') + '\\temp'
    )

    if (path.length > 1 && path.endsWith('\\') && !path.endsWith(':\\')) {
      path = path.slice(0, -1)
    }
  } else {
    path = (
      globalThis.__args.env.TEMPDIR ||
      globalThis.__args.env.TMPDIR ||
      globalThis.__args.env.TEMP ||
      globalThis.__args.env.TMP ||
      ''
    )

    // derive default
    if (!path) {
      if (platform() === 'ios') {
        // @TODO(jwerle): use a path module
        path = [primordials.cwd, 'tmp'].join('/')
      } else if (platform() === 'android') {
        path = '/data/local/tmp'
      } else {
        path = '/tmp'
      }
    }

    if (path.length > 1 && path.endsWith('/')) {
      path = path.slice(0, -1)
    }
  }

  return path
}

export const EOL = (() => {
  if (isWindows()) {
    return '\r\n'
  }

  return '\n'
})()

export function rusage () {
  const { err, data } = ipc.sendSync('os.rusage')
  if (err) throw err
  return data
}

export function uptime () {
  const { err, data } = ipc.sendSync('os.uptime')
  if (err) throw err
  return data
}

export function uname () {
  if (!cache.uname) {
    const { err, data } = ipc.sendSync('os.uname')
    if (err) throw err
    cache.uname = data
  }

  return cache.uname
}

export function hrtime () {
  const result = ipc.sendSync('os.hrtime', {}, {
    desiredResponseType: 'arraybuffer'
  })

  if (result.err) throw result.err
  return result.data.readBigUInt64BE(0)
}

export function availableMemory () {
  const result = ipc.sendSync('os.availableMemory', {}, {
    desiredResponseType: 'arraybuffer'
  })

  if (result.err) throw result.err
  return result.data.readBigUInt64BE(0)
}

// eslint-disable-next-line
import * as exports from './os.js'
export default exports
