import * as os from '../../os.js'
import { test } from '@socketsupply/tapzero'

const archs = ['arm64', 'ia32', 'x64', 'unknown']
const platforms = ['android', 'cygwin', 'freebsd', 'linux', 'darwin', 'ios', 'openbsd', 'win32', 'unknown']
const types = ['CYGWIN_NT', 'Mac', 'Darwin', 'FreeBSD', 'Linux', 'OpenBSD', 'Windows_NT', 'Unknown']

test('os.arch()', (t) => {
  t.ok(archs.includes(os.arch()), 'os.arch() value is valid')
})

test('os.platform()', (t) => {
  t.ok(platforms.includes(os.platform()), 'os.platform()')
})

test('os.type()', (t) => {
  t.ok(types.includes(os.type()), 'os.type()')
})

test('os.networkInterfaces()', (t) => {
  function isValidAddress (address) {
    return Object.keys(address).toString() === ['address', 'netmask', 'internal', 'family', 'cidr', 'mac'].toString()
  }

  const networkInterfaces = os.networkInterfaces()
  t.ok(Array.isArray(networkInterfaces.lo) || Array.isArray(networkInterfaces.lo0), 'iterface is "lo"')
  const interfaces = Object.values(networkInterfaces)
  t.ok(interfaces.length >= 2, 'network interfaces has at least two keys, loopback + wifi, was:' + interfaces.length)
  t.ok(interfaces.every(addresses => Array.isArray(addresses)), 'network interface is an array')
  t.ok(
    interfaces.every(addresses => addresses.every(isValidAddress)),
    'all network interfaces addresses has correct keys'
  )
})

test('os.EOL', (t) => {
  if (/windows/i.test(os.type())) {
    t.equal(os.EOL, '\r\n')
  } else {
    t.equal(os.EOL, '\n')
  }
})
