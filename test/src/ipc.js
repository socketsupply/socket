import * as ipc from '../../ipc.js'
import { test } from '@socketsupply/tapzero'
import { Buffer } from '../../buffer.js'

// node compat
// import { Buffer } from 'node:buffer'
// import './test-context.js'

test('ipc exports', async (t) => {
  t.deepEqual(Object.keys(ipc).sort(), [
    'OK',
    'Result',
    'TIMEOUT',
    'createBinding',
    'debug',
    'default',
    'emit',
    'ERROR',
    'kDebugEnabled',
    'Message',
    'parseSeq',
    'postMessage',
    'ready',
    'request',
    'resolve',
    'send',
    'sendSync',
    'write'
  ].sort())

  try {
    await ipc.ready()
  } catch (err) {
    t.fail(err)
  }
})

test('ipc constants', (t) => {
  t.equal(ipc.OK, 0)
  t.equal(ipc.ERROR, 1)
  t.equal(ipc.TIMEOUT, 32000)
  t.equal(ipc.kDebugEnabled, Symbol.for('ipc.debug.enabled'))
})

test('ipc.debug', (t) => {
  ipc.debug(true)
  t.equal(ipc.debug.enabled, true)
  ipc.debug(false)
  t.equal(ipc.debug.enabled, false)
  ipc.debug(true)
})

test('ipc.Message', (t) => {
  t.ok(ipc.Message.prototype instanceof URL, 'is a URL')
  // pass a Buffer
  let msg = ipc.Message.from(Buffer.from('test'), { foo: 'bar' })
  t.equal(msg.protocol, ipc.Message.PROTOCOL)
  t.equal(msg.command, 'test')
  t.deepEqual(msg.params, { foo: 'bar' })
  // pass an ipc.Message
  msg = ipc.Message.from(msg)
  t.equal(msg.protocol, ipc.Message.PROTOCOL)
  t.equal(msg.command, 'test')
  t.deepEqual(msg.params, { foo: 'bar' })
  // pass an object
  msg = ipc.Message.from({ protocol: ipc.Message.PROTOCOL, command: 'test' }, { foo: 'bar' })
  t.equal(msg.protocol, ipc.Message.PROTOCOL)
  t.equal(msg.command, 'test')
  t.deepEqual(msg.params, { foo: 'bar' })
  // pass a string
  msg = ipc.Message.from('test', { foo: 'bar' })
  t.equal(msg.protocol, ipc.Message.PROTOCOL)
  t.equal(msg.command, 'test')
  t.deepEqual(msg.params, { foo: 'bar' })
  t.ok(ipc.Message.isValidInput(`${ipc.Message.PROTOCOL}//test`), 'is valid input')
  t.ok(!ipc.Message.isValidInput('test'), 'is valid input')
  t.ok(!ipc.Message.isValidInput('foo://test'), 'is valid input')
})

if (window.__args.os !== 'ios' && window.__args.os !== 'android') {
  test('ipc.sendSync not found', (t) => {
    const response = ipc.sendSync('test', { foo: 'bar' })
    t.ok(response instanceof ipc.Result)
    const { err } = response
    t.equal(err?.toString(), 'NotFoundError: Not found')
    t.equal(err?.name, 'NotFoundError')
    t.equal(err?.message, 'Not found')
    t.ok(err?.url.startsWith(`${ipc.Message.PROTOCOL}//test?foo=bar&index=0&seq=R`))
    t.equal(err?.code, 'NOT_FOUND_ERR')
  })

  test('ipc.sendSync success', (t) => {
    const response = ipc.sendSync('os.arch')
    t.ok(response instanceof ipc.Result)
    const { data } = response
    t.ok(['x86_64', 'arm64'].includes(data))
  })

  //
  // TODO: ipc.send error should match ipc.sendSync error
  //
  test('ipc.send not found', async (t) => {
    const response = await ipc.send('test', { foo: 'bar' })
    t.ok(response instanceof ipc.Result, 'response is an ipc.Result')
    t.ok(response.err instanceof Error, 'response.err is an Error')
    t.equal(response.err.toString(), 'Error: unsupported IPC message: test')
    t.equal(response.err.name, 'Error')
    t.equal(response.err.message, 'unsupported IPC message: test')
  })

  test('ipc.send success', async (t) => {
    const response = await ipc.send('os.arch')
    t.ok(response instanceof ipc.Result)
    const { data } = response
    t.ok(['x86_64', 'arm64'].includes(data))
  })
}
