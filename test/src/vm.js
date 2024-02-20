import test from 'socket:test'
import vm from 'socket:vm'

test('vm.runInContext(source) - simple', async (t) => {
  t.equal(await vm.runInContext('1 + 2 + 3'), 6, 'vm.runInContext("1 + 2 + 3")')
  t.deepEqual(
    await vm.runInContext('{ number: 123 }'),
    { number: 123 },
    'vm.runInContext("{ number: 123 }")'
  )
})

test('vm.runInContext(source) - script refererences', async (t) => {
  async function identity (...args) {
    return args
  }

  const result = await vm.runInContext(identity)
  t.deepEqual(await result(1, 2, 3), [1, 2, 3], 'function reference returns value')

  const FunctionReference = await vm.runInContext('Function')
  const fn = new FunctionReference('return 123')

  t.equal(123, await fn(), 'Function constructor with new call')

  const Class = await vm.runInContext('class Class { method (...args) { return args } }')
  t.equal('function', typeof Class, 'Class constructor')

  const instance = new Class()

  t.deepEqual([1, 2, 3], await instance.method(1, 2, 3), 'class instance method')
})

test('vm.runInContext(source, context) - context', async (t) => {
  const context = {
    key: 'value',
    functions: {}
  }

  const value = await vm.runInContext('key', { context })
  t.equal(context.key, value, 'context.key === value')

  await vm.runInContext('key = "other value"', { context })
  t.equal(context.key, 'other value', 'context.key === "other value"')

  await vm.runInContext('functions.hello = () => "hello world"', { context })
  t.equal('function', typeof context.functions.hello, 'typeof context.function.hello === "function"')
  t.equal('hello world', await context.functions.hello(), 'await context.function.hello() === "hello world"')
})

test('vm.runInContext(source, context) - transferables', async (t) => {
  const channel = new MessageChannel()
  const context = {
    buffer: new TextEncoder().encode('hello world'),
    scope: {}
  }

  await vm.runInContext(`
    let port = null

    scope.setMessagePort = (value) => {
      port = value
      port.onmessage = (event) => port.postMessage(event.data)
    }

    scope.decoded = new TextDecoder().decode(buffer)
  `, { context })

  await context.scope.setMessagePort(channel.port2)
  t.equal('hello world', context.scope.decoded, 'context.scope.decoded === "hello world"')
  channel.port1.postMessage(context.scope.decoded)
  await new Promise((resolve) => {
    channel.port1.onmessage = (event) => {
      t.equal('hello world', event.data, 'port1 message is "hello world"')
      resolve()
    }
  })
})

test('vm.runInContext(source, context) - ESM', async (t) => {
  const module = await vm.runInContext(`
    const storage = {}
    export function set (key, value) {
      storage[key] = value
    }

    export function get (key) {
      return storage[key]
    }
  `)

  t.equal('function', typeof module.set, 'typeof module.set === "function"')
  t.equal('function', typeof module.get, 'typeof module.set === "function"')
  await module.set('key', 'value')
  t.equal('value', await module.get('key'), 'module.get("key") === "value"')
})
