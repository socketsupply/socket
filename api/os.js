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

/**
 * Returns the operating system CPU architecture for which Socket was compiled.
 * @returns {string} - 'arm64', 'ia32', 'x64', or 'unknown'
 */
export function arch () {
  return primordials.arch
}

/**
 * Returns an array of objects containing information about each CPU/core.
 * @returns {Array<object>} cpus - An array of objects containing information about each CPU/core.
 * The properties of the objects are:
 * - model `<string>` - CPU model name.
 * - speed `<number>` - CPU clock speed (in MHz).
 * - times `<object>` - An object containing the fields user, nice, sys, idle, irq representing the number of milliseconds the CPU has spent in each mode.
 *   - user `<number>` - Time spent by this CPU or core in user mode.
 *   - nice `<number>` - Time spent by this CPU or core in user mode with low priority (nice).
 *   - sys `<number>` - Time spent by this CPU or core in system mode.
 *   - idle `<number>` - Time spent by this CPU or core in idle mode.
 *   - irq `<number>` - Time spent by this CPU or core in IRQ mode.
 * @see {@link https://nodejs.org/api/os.html#os_os_cpus}
 */
export function cpus () {
  if (!cache.cpus) {
    const { err, data } = ipc.sendSync('os.cpus')
    if (err) throw err
    cache.cpus = data
  }

  return cache.cpus
}

/**
 * Returns an object containing network interfaces that have been assigned a network address.
 * @returns {object}  - An object containing network interfaces that have been assigned a network address.
 * Each key on the returned object identifies a network interface. The associated value is an array of objects that each describe an assigned network address.
 * The properties available on the assigned network address object include:
 * - address `<string>` - The assigned IPv4 or IPv6 address.
 * - netmask `<string>` - The IPv4 or IPv6 network mask.
 * - family `<string>` - The address family ('IPv4' or 'IPv6').
 * - mac `<string>` - The MAC address of the network interface.
 * - internal `<boolean>` - Indicates whether the network interface is a loopback interface.
 * - scopeid `<number>` - The numeric scope ID (only specified when family is 'IPv6').
 * - cidr `<string>` - The CIDR notation of the interface.
 * @see {@link https://nodejs.org/api/os.html#os_os_networkinterfaces}
 */
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

/**
 * Returns the operating system platform.
 * @returns {string} - 'android', 'cygwin', 'freebsd', 'linux', 'darwin', 'ios', 'openbsd', 'win32', or 'unknown'
 * @see {@link https://nodejs.org/api/os.html#os_os_platform}
 * The returned value is equivalent to `process.platform`.
 */
export function platform () {
  return primordials.platform
}

/**
 * Returns the operating system name.
 * @returns {string} - 'CYGWIN_NT', 'Mac', 'Darwin', 'FreeBSD', 'Linux', 'OpenBSD', 'Windows_NT', 'Win32', or 'Unknown'
 * @see {@link https://nodejs.org/api/os.html#os_os_type}
 */
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

/**
 * @returns {boolean} - `true` if the operating system is Windows.
 */
export function isWindows () {
  if (cache.isWindows) {
    return cache.isWindows
  }

  cache.isWindows = /^win.*/i.test(type())
  return cache.isWindows
}

/**
 * @returns {string} - The operating system's default directory for temporary files.
 */
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

/**
 * @type {string}
 * The operating system's end-of-line marker. `'\r\n'` on Windows and `'\n'` on POSIX.
 */
export const EOL = (() => {
  if (isWindows()) {
    return '\r\n'
  }

  return '\n'
})()

// FIXME: should be process.resourceUsage()
/**
 * Get resource usage.
 */
export function rusage () {
  const { err, data } = ipc.sendSync('os.rusage')
  if (err) throw err
  return data
}

/**
 * Returns the system uptime in seconds.
 * @returns {number} - The system uptime in seconds.
 */
export function uptime () {
  const { err, data } = ipc.sendSync('os.uptime')
  if (err) throw err
  return data
}

// FIXME: should be os.machine() + os.release() + os.type() + os.version()
/**
 * Returns the operating system name.
 * @returns {string} - The operating system name.
 */
export function uname () {
  if (!cache.uname) {
    const { err, data } = ipc.sendSync('os.uname')
    if (err) throw err
    cache.uname = data
  }

  return cache.uname
}

/**
 * It's implemented in process.hrtime.bigint()
 * @ignore
 */
export function hrtime () {
  const result = ipc.sendSync('os.hrtime', {}, {
    desiredResponseType: 'arraybuffer'
  })

  if (result.err) throw result.err
  return result.data.readBigUInt64BE(0)
}

/**
 * Node.js doesn't have this method.
 * @ignore
 */
export function availableMemory () {
  const result = ipc.sendSync('os.availableMemory', {}, {
    desiredResponseType: 'arraybuffer'
  })

  if (result.err) throw result.err
  return result.data.readBigUInt64BE(0)
}

/**
 * The host operating system. This value can be one of:
 * - android
 * - android-emulator
 * - iphoneos
 * - iphone-simulator
 * - linux
 * - macosx
 * - unix
 * - unknown
 * - win32
 * @ignore
 * @return {'android'|'android-emulator'|'iphoneos'|iphone-simulator'|'linux'|'macosx'|unix'|unknown'|win32'}
 */
export function host () {
  return primordials['host-operating-system'] || 'unknown'
}

// eslint-disable-next-line
import * as exports from './os.js'
export default exports
