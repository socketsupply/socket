import { compareBuffers } from 'socket:util'
import { createFile } from 'socket:fs/web'
import { Buffer } from 'socket:buffer'
import { test } from 'socket:test'
import process from 'socket:process'
import path from 'socket:path'
import fs from 'socket:fs/promises'
import os from 'socket:os'

let TMPDIR = null
let FIXTURES = null

if (process.platform !== 'ios') {
  TMPDIR = `${os.tmpdir()}${path.sep}`
  FIXTURES = /android/i.test(os.platform())
    ? '/data/local/tmp/ssc-socket-test-fixtures/'
    : `${TMPDIR}ssc-socket-test-fixtures${path.sep}`
}

test('createFile - simple', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const file = await createFile(filename, { highWaterMark: 256 })
  t.ok(file && file instanceof globalThis.File, 'File instance is created')
})

test('createFile - File.prototype.arrayBuffer()', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const handle = await fs.open(filename)
  const file = await createFile(filename, { highWaterMark: 256 })
  const arrayBuffer = await file.arrayBuffer()
  const buffer = await handle.readFile()
  t.ok(compareBuffers(arrayBuffer, buffer) === 0, 'arrayBuffer() returns actual contents')
  await handle.close()
})

test('createFile - File.prototype.text()', async (t) => {
  const filename = FIXTURES + 'file.txt'
  const handle = await fs.open(filename)
  const file = await createFile(filename)
  const text = await file.text()
  const buffer = await handle.readFile()
  const encoder = new TextEncoder()
  t.ok(compareBuffers(encoder.encode(text), buffer) === 0, 'text() returns actual contents')
  await handle.close()
})

test('createFile - File.prototype.stream()', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const handle = await fs.open(filename)
  const file = await createFile(filename, { highWaterMark: 256 })
  const stream = file.stream()
  const buffer = await handle.readFile()
  const chunks = []
  for await (const chunk of stream) {
    chunks.push(chunk)
  }
  t.ok(compareBuffers(Buffer.concat(chunks), buffer) === 0, 'stream() returns actual contents that are chunked')
  await handle.close()
})

/**
 * TODO

test('createFileSystemFileHandle', async (t) => {
})

test('createFileSystemDirectoryHandle', async (t) => {
})

*/
