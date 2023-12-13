import { test } from 'socket:test'
import fs from 'socket:fs'
import { createFile } from 'socket:fs/web'

const importObject = {
  env: {
    myFunc: arg => arg * arg
  }
}

test('load wasm synchronously', async (t) => {
  const wasm = await fs.promises.readFile('./webassembly/program.wasm')
  const module = new WebAssembly.Module(wasm)
  const { exports } = new WebAssembly.Instance(module, importObject)
  t.equal(exports.runMyFunc(5), 25, 'imported function works')
  t.equal(exports.identity(42), 42, 'exported function works')
})

test('load wasm asynchronously', async (t) => {
  const file = await createFile('./webassembly/program.wasm')
  const stream = file.stream()
  const response = new Response(stream, {
    headers: {
      'Content-Type': 'application/wasm'
    }
  })
  const { instance } = await WebAssembly.instantiateStreaming(response, importObject)
  t.equal(instance.exports.runMyFunc(5), 25, 'imported function works')
  t.equal(instance.exports.identity(42), 42, 'exported function works')
})
