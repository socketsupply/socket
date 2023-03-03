import { test } from 'socket:test'
import process from 'socket:process'
import { primordials } from 'socket:ipc'
import path from 'path-browserify'

test('process', (t) => {
  t.ok(typeof process.addListener === 'function', 'process is an EventEmitter')
})

test('process.homedir()', (t) => {
  t.ok(typeof process.homedir() === 'string', 'process.homedir() returns a string')
})

test('process.exit()', (t) => {
  t.ok(typeof process.exit === 'function', 'process.exit() is a function')
})

test('process.cwd', async (t) => {
  // make `path-browserify` happy
  globalThis.process = process

  t.equal(typeof process.cwd(), 'string', 'process.cwd() returns a string')
  t.equal(process.cwd(), primordials.cwd, 'process.cwd() equals primordials.cwd')
  if (process.platform === 'darwin' || process.platform === 'ios') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../Resources'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'linux') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../socket-runtime-javascript-tests'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'android') {
    // t.ok(process.cwd(), 'process.cwd() returns a correct value')
  } else if (process.platform === 'win32') {
    // TODO(trevnorris): Fix to use path once implemented for Windows
    t.equal(process.cwd(), process.argv0.slice(0, process.argv0.lastIndexOf('\\') + 1), 'process.cwd() returns a correct value')
  } else {
    // TODO: iOS
    t.fail(`FIXME: not implemented for platform ${process.platform}`)
  }

  // make us happy
  delete globalThis.process
})

test('process.arch', (t) => {
  t.ok(['x64', 'arm64'].includes(process.arch), 'process.arch is correct')
  t.equal(process.arch, primordials.arch, 'process.arch equals primordials.arch')
})

test('process.platform', (t) => {
  t.ok(typeof process.platform === 'string', 'process.platform returns an string')
  t.ok(['darwin', 'freebsd', 'linux', 'openbsd', 'sunos', 'win32', 'android', 'ios'].includes(process.platform), 'process.platform is correct')
  t.equal(process.platform, primordials.platform, 'process.platform equals primordials.platform')
})

test('process.env', (t) => {
  t.deepEqual(process.env, globalThis.__args.env, 'process.env is equal to globalThis.__args.env')
})

test('process.argv', (t) => {
  t.deepEqual(process.argv, globalThis.__args.argv, 'process.argv is equal to globalThis.__args.argv')
})

test('os.hrtime()', (t) => {
  const hrtime = process.hrtime()
  t.ok(Array.isArray(hrtime), 'process.hrtime() returns array')
  t.ok(hrtime?.length === 2, 'process.hrtime() returns array of length 2')
  t.equal(typeof hrtime[0], 'number', 'hrtime[0] is a number')
  t.equal(typeof hrtime[1], 'number', 'hrtime[1] is a number')
})

test('os.hrtime.bigint()', (t) => {
  const hrtime = process.hrtime.bigint()
  t.ok(hrtime > 0, 'hrtime > 0')
  t.equal(typeof hrtime, 'bigint', 'hrtime is bigint')
})
