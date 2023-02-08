/**
 * @module OS
 *
 * This module provides normalized system information from all the major
 * operating systems.
 */

import { toProperCase } from './util.js'
import process from './process.js'
import ipc from './ipc.js'

const UNKNOWN = 'unknown'

const cache = {
  arch: UNKNOWN,
  type: UNKNOWN,
  platform: UNKNOWN
}

export function arch () {
  let value = UNKNOWN

  if (cache.arch !== UNKNOWN) {
    return cache.arch
  }

  if (typeof window !== 'object') {
    if (typeof process?.arch === 'string') {
      return process.arch
    }
  }

  if (typeof window === 'object') {
    value = (
      process.arch ||
      ipc.sendSync('os.arch')?.data ||
      UNKNOWN
    )
  }

  if (value === 'arm64') {
    return value
  }

  cache.arch = value
    .replace('x86_64', 'x64')
    .replace('x86', 'ia32')
    .replace(/arm.*/, 'arm')

  return cache.arch
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

    if (address === '127.0.0.1' || address === '0.0.0.0') {
      internal = true
      mac = '00:00:00:00:00:00'

      if (address === '127.0.0.1') {
        cidr = '127.0.0.1/8'
        netmask = '255.0.0.0'
      } else {
        cidr = '0.0.0.0/0'
        netmask = '0.0.0.0'
      }
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
  let value = UNKNOWN

  if (cache.platform !== UNKNOWN) {
    return cache.platform
  }

  if (typeof window !== 'object') {
    if (typeof process?.platform === 'string') {
      return process.platform.toLowerCase()
    }
  }

  if (typeof window === 'object') {
    value = (
      process.os ||
      ipc.sendSync('os.platform')?.data ||
      platform?.platform ||
      UNKNOWN
    )
  }

  cache.platform = value
    .replace(/^mac/i, 'darwin')
    .toLowerCase()

  return cache.platform
}

export function type () {
  let value = 'unknown'

  if (cache.type !== UNKNOWN) {
    return cache.type
  }

  if (typeof window !== 'object') {
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

  if (typeof window === 'object') {
    value = (
      platform?.platform ||
      ipc.sendSync('os.type')?.data ||
      UNKNOWN
    )
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
        path = [process.cwd(), 'tmp'].join('/')
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
