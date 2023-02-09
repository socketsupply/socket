import { test } from 'socket:test'
import process from 'socket:process'
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
  t.equal(typeof process.cwd(), 'string', 'process.cwd() returns a string')
  if (process.platform === 'mac') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../Resources'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'linux') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../socket-runtime-javascript-tests'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'android') {
    t.ok(process.cwd(), 'process.cwd() returns a correct value')
  } else if (process.platform === 'win32') {
    // TODO(trevnorris): Fix to use path once implemented for Windows
    t.equal(process.cwd(), process.argv0.slice(0, process.argv0.lastIndexOf('\\') + 1), 'process.cwd() returns a correct value')
  } else {
    // TODO: iOS
    t.fail(`FIXME: not implemented for platform ${process.platform}`)
  }
})

test('process.arch', (t) => {
  t.ok(['x86_64', 'arm64'].includes(process.arch), 'process.arch is correct')
  t.equal(process.arch, window.__args.arch, 'process.arch equals window.__args.arch')
})

test('process.platform', (t) => {
  t.ok(typeof process.platform === 'string', 'process.platform returns an string')
  t.ok(['mac', 'linux', 'android', 'ios', 'win32'].includes(process.platform), 'process.platform is correct')
  t.equal(process.platform, window.__args.os, 'process.platform equals window.__args.platform')
})

test('process.env', (t) => {
  t.deepEqual(process.env, window.__args.env, 'process.env is equal to window.__args.env')
})

test('process.argv', (t) => {
  t.deepEqual(process.argv, window.__args.argv, 'process.argv is equal to window.__args.argv')
})
