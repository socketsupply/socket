/**
 * @module OS
 *
 * This module provides normalized system information from all the major
 * operating systems.
 */

import { toProperCase } from './util.js'
import ipc, { primordials } from './ipc.js'

const UNKNOWN = 'unknown'

const cache = {
  type: UNKNOWN
}

export function arch () {
  return primordials.arch
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

  if (globalThis !== window) {
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

  if (globalThis === window) {
    value = ipc.sendSync('os.type')?.data ?? UNKNOWN
  }

  value = value.replace(/android/i, 'Linux')
  value = value.replace(/ios/i, 'Darwin')

  if (value !== UNKNOWN) {
    value = toProperCase(value)
  }

  cache.type = value

  return cache.type
}

export function isWindows () {
  if ('isWindows' in cache) {
    return cache.isWindows
  }

  cache.isWindows = /^win.*/i.test(type())
  return cache.isWindows
}

export function tmpdir () {
  let path = ''

  if (isWindows()) {
    path = (
      process?.env?.TEMPDIR ||
      process?.env?.TMPDIR ||
      process?.env?.TEMP ||
      process?.env?.TMP ||
      (process?.env?.SystemRoot || process?.env?.windir || '') + '\\temp'
    )

    if (path.length > 1 && path.endsWith('\\') && !path.endsWith(':\\')) {
      path = path.slice(0, -1)
    }
  } else {
    path = (
      process?.env?.TEMPDIR ||
      process?.env?.TMPDIR ||
      process?.env?.TEMP ||
      process?.env?.TMP ||
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

// eslint-disable-next-line
import * as exports from './os.js'
export default exports
