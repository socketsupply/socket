import { test } from '@socketsupply/tapzero'
import process from '../../process.js'
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
  t.ok(typeof process.cwd() === 'string', 'process.cwd() returns a string')
  if (process.platform === 'mac') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../Resources'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'linux') {
    t.equal(process.cwd(), path.resolve(process.argv0, '../../socketsupply-socket-tests'), 'process.cwd() returns a correct value')
  } else if (process.platform === 'android') {
    t.ok(process.cwd(), 'process.cwd() returns a correct value')
  } else {
    // TODO: iOS, Windows
    t.fail(`FIXME: not implemented for platform ${process.platform}`)
  }
})

test('process.arch', (t) => {
  t.ok(['x86_64', 'arm64'].includes(process.arch), 'process.arch is correct')
  t.equal(process.arch, window.__args.arch, 'process.arch equals window.__args.arch')
})

test('process.platform', (t) => {
  t.ok(typeof process.platform === 'string', 'process.platform returns an string')
  t.ok(['mac', 'linux', 'android', 'ios', 'win'].includes(process.platform), 'process.platform is correct')
  t.equal(process.platform, window.__args.os, 'process.platform equals window.__args.platform')
  t.equal(process.platform, process.os, 'process.platform returns the same value as process.os')
})

test('process.env', (t) => {
  t.deepEqual(process.env, window.__args.env, 'process.env is equal to window.__args.env')
})

test('process.argv', (t) => {
  t.deepEqual(process.argv, window.__args.argv, 'process.argv is equal to window.__args.argv')
})
