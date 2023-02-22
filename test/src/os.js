import { primordials } from 'socket:ipc'
import { test } from 'socket:test'
import * as os from 'socket:os'

const archs = ['arm64', 'ia32', 'x64', 'unknown']
const platforms = ['android', 'cygwin', 'freebsd', 'linux', 'darwin', 'ios', 'openbsd', 'win32', 'unknown']
const types = ['CYGWIN_NT', 'Mac', 'Darwin', 'FreeBSD', 'Linux', 'OpenBSD', 'Windows_NT', 'Win32', 'Unknown']

test('os.arch()', (t) => {
  t.ok(archs.includes(os.arch()), 'os.arch() value is valid')
})

test('os.cpus()', (t) => {
  const cpus = os.cpus()
  t.ok(Array.isArray(cpus), 'Array.isArray(cpus)')
  t.ok(cpus.length > 0, 'cpus.length > 0')
  t.equal(typeof cpus[0].model, 'string', 'cpus[0].model is string')
  t.equal(typeof cpus[0].speed, 'number', 'cpus[0].numberl is number')
  t.equal(cpus[0].times !== null && typeof cpus[0].times, 'object', 'cpus[0].times is object')
  t.equal(typeof cpus[0].times.user, 'number', 'cpus[0].times.user is number')
  t.equal(typeof cpus[0].times.nice, 'number', 'cpus[0].times.nice is number')
  t.equal(typeof cpus[0].times.sys, 'number', 'cpus[0].times.sys is number')
  t.equal(typeof cpus[0].times.idle, 'number', 'cpus[0].times.idle is number')
  t.equal(typeof cpus[0].times.irq, 'number', 'cpus[0].times.irq is number')
})

test('os.platform()', (t) => {
  t.ok(platforms.includes(os.platform()), 'os.platform() value is valid')
  t.equal(os.platform(), primordials.platform, 'os.platform() equals primordials.platform')
})

test('os.type()', (t) => {
  t.ok(types.includes(os.type()), 'os.type() value is valid')
})

test('os.isWindows()', (t) => {
  t.equal(os.isWindows(), primordials.platform === 'win32', 'os.isWindows() value is valid')
})

test('os.tmpdir()', (t) => {
  t.equal(typeof os.tmpdir(), 'string', 'os.type() value is a string')
  t.ok(os.tmpdir().length > 0, 'tmpdir length > 0')
})

test('os.networkInterfaces()', (t) => {
  function isValidAddress (address) {
    return Object.keys(address).toString() === ['address', 'netmask', 'internal', 'family', 'cidr', 'mac'].toString()
  }

  const networkInterfaces = os.networkInterfaces()
  const lo = os.platform() === 'win32' ? 'Loopback Pseudo-Interface 1' : 'lo'
  t.ok(Array.isArray(networkInterfaces[lo]) || Array.isArray(networkInterfaces.lo0), 'iterface is "lo"')
  const interfaces = Object.values(networkInterfaces)
  t.ok(interfaces.length >= 2, 'network interfaces has at least two keys, loopback + wifi, was:' + interfaces.length)
  t.ok(interfaces.every(addresses => Array.isArray(addresses)), 'network interface is an array')
  t.ok(
    interfaces.every(addresses => addresses.every(isValidAddress)),
    'all network interfaces addresses has correct keys'
  )
})

test('os.EOL', (t) => {
  if (os.platform() === 'win32') {
    t.equal(os.EOL, '\r\n')
  } else {
    t.equal(os.EOL, '\n')
  }
})
