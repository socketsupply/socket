import extension from 'socket:extension'
import test from 'socket:test'
import ipc from 'socket:ipc'

test('extension.load(name) - simple', async (t) => {
  const stats = await extension.stats()
  try {
    const simple = await extension.load('simple-ipc-ping')
    const result = await ipc.request('simple.ping', { value: 'hello world' })
    t.equal(simple.abi, stats.abi, 'simple.abi === stats.abi')
    t.equal(simple.name, 'simple-ipc-ping', 'name === "simple-ipc-ping"')
    t.equal(simple.description, 'a simple IPC ping extension', 'description === "a simple IPC ping extension"')
    t.equal(simple.version, '0.1.2', 'version === "0.1.2"')
    t.equal(result.data, 'hello world', 'ipc://simple.ping mapped')
  } catch (err) {
    t.ifError(err)
  }
})
