import { compareBuffers } from 'socket:util'
import { Buffer } from 'socket:buffer'
import { test } from 'socket:test'
import process from 'socket:process'
import path from 'socket:path'
import mime from 'socket:mime'
import fs from 'socket:fs/promises'
import os from 'socket:os'

import {
  createFile,
  createFileSystemFileHandle,
  createFileSystemDirectoryHandle
} from 'socket:fs/web'

const TMPDIR = `${os.tmpdir()}${path.sep}`
const FIXTURES = /android/i.test(os.platform())
  ? '/data/local/tmp/ssc-socket-test-fixtures/'
  : `${TMPDIR}ssc-socket-test-fixtures${path.sep}`

test('createFile - simple', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const fd = await fs.open(filename)
  const stats = await fd.stat()
  const types = await mime.lookup(path.extname(filename).slice(1))
  const type = types[0]?.mime ?? ''
  const file = await createFile(filename, { highWaterMark: 256 })

  t.ok(file && file instanceof globalThis.File, 'File instance is created')
  t.equal(Number(file.lastModifiedDate), Number(new Date(stats.mtimeMs)), 'file.lastModifiedDate')
  t.equal(file.lastModified, stats.mtimeMs, 'file.lastModified')
  t.equal(file.name, path.basename(filename), 'file.name')
  t.equal(file.size, stats.size, 'file.size')
  t.equal(file.type, type, 'file.type')
})

test('createFile - File.prototype.arrayBuffer()', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const fd = await fs.open(filename)
  const file = await createFile(filename, { highWaterMark: 256 })
  const arrayBuffer = await file.arrayBuffer()
  const buffer = await fd.readFile()
  t.ok(compareBuffers(arrayBuffer, buffer) === 0, 'arrayBuffer() returns actual contents')
  await fd.close()
})

test('createFile - File.prototype.text()', async (t) => {
  const filename = FIXTURES + 'file.txt'
  const fd = await fs.open(filename)
  const file = await createFile(filename)
  const text = await file.text()
  const buffer = await fd.readFile()
  const encoder = new TextEncoder()
  t.ok(compareBuffers(encoder.encode(text), buffer) === 0, 'text() returns actual contents')
  await fd.close()
})

test('createFile - File.prototype.stream()', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const fd = await fs.open(filename)
  const file = await createFile(filename, { highWaterMark: 256 })
  const stream = file.stream()
  const buffer = await fd.readFile()
  const chunks = []
  for await (const chunk of stream) {
    chunks.push(chunk)
  }
  t.ok(compareBuffers(Buffer.concat(chunks), buffer) === 0, 'stream() returns actual contents that are chunked')
  await fd.close()
})

test('createFileSystemFileHandle', async (t) => {
  const filename = FIXTURES + 'data.bin'
  const file = await createFile(filename)
  const handle = await createFileSystemFileHandle(file)
  const copy = await createFileSystemFileHandle(file)

  t.ok(handle && handle instanceof globalThis.FileSystemFileHandle, 'FileSystemFileHandle instance created')
  t.equal(file, handle.getFile(), 'handle.getFile()')
  t.equal(handle.kind, 'file', 'handle.kind')
  t.equal(handle.name, path.basename(filename), 'handle.name')

  t.ok(await handle.isSameEntry(handle), 'handle.isSameEntry() on same instance')
  t.ok(await handle.isSameEntry(copy), 'handle.isSameEntry() on copy instance')
})

test('createFileSystemDirectoryHandle', async (t) => {
  const dirname = FIXTURES + 'directory'
  const handle = await createFileSystemDirectoryHandle(dirname)
  const copy = await createFileSystemDirectoryHandle(dirname)

  t.ok(handle && handle instanceof globalThis.FileSystemDirectoryHandle, 'FileSystemDirectoryHandle instance created')
  t.equal(handle.kind, 'directory', 'handle.kind')
  t.equal(handle.name, path.basename(dirname), 'handle.name')

  t.ok(await handle.isSameEntry(handle), 'handle.isSameEntry() on same instance')
  t.ok(await handle.isSameEntry(copy), 'handle.isSameEntry() on copy instance')

  const entries = []
  for await (const entry of handle.entries()) {
    entries.push(entry)
  }

  const items = Object.fromEntries(entries)
  const expected = [
    '0.txt',
    '1.txt',
    '2.txt',
    'a.txt',
    'b.txt',
    'c.txt'
  ]

  for (const key of expected) {
    t.ok(items[key] instanceof globalThis.FileSystemHandle, `'${key}' is FileSystemHandle`)
  }
})
