import extension from 'socket:extension'
import test from 'socket:test'

test('extension.load(name) - wasm', async (t) => {
  const stats = await extension.stats()
  const wasm = await extension.load('wasm')

  t.ok(wasm, 'wasm = extension.load()')
  t.equal(wasm.abi, stats.abi, 'wasm.abi === stats.abi')
  t.equal(wasm.name, 'wasm', 'name === "wasm"')
  t.equal(wasm.description, 'A native extension compiled to WASM')
  t.equal(wasm.version, '0.0.1', 'version === "0.0.1"')

  globalThis.wasm = wasm
})
