import fs from '../../../fs/promises.js'
import os from '../../../os.js'
import path from '../../../path.js'
import Buffer from '../../../buffer.js'

import { test } from '@socketsupply/tapzero'
import { FileHandle } from '../../../fs/handle.js'
import { Dir } from '../../../fs/dir.js'

const TMPDIR = `${os.tmpdir()}${path.sep}`
const FIXTURES = /android/i.test(os.platform())
  ? '/data/local/tmp/ssc-socket-test-fixtures/'
  : `${TMPDIR}ssc-socket-test-fixtures${path.sep}`

test('fs.promises.access', async (t) => {
  let access = await fs.access(FIXTURES, fs.constants.F_OK)
  t.equal(access, true, '(F_OK) fixtures/ directory is accessible')

  access = await fs.access(FIXTURES, fs.constants.F_OK | fs.constants.R_OK)
  t.equal(access, true, '(F_OK | R_OK) fixtures/ directory is readable')

  access = await fs.access('.', fs.constants.W_OK)
  t.equal(access, true, '(W_OK) ./ directory is writable')

  access = await fs.access(FIXTURES, fs.constants.X_OK)
  t.equal(access, true, '(X_OK) fixtures/ directory is "executable" - can list items')
})

test('fs.promises.chmod', async (t) => {
  const chmod = await fs.chmod(FIXTURES + 'file.txt', 0o777)
  t.equal(chmod, undefined, 'file.txt is chmod 777')
})

test('fs.promises.mkdir', async (t) => {
  const dirname = FIXTURES + Math.random().toString(16).slice(2)
  const { err } = fs.mkdir(dirname, {})
  t.equal(err, undefined, 'mkdir does not throw')
})

test('fs.promises.open', async (t) => {
  const fd = await fs.open(FIXTURES + 'file.txt', 'r')
  t.ok(fd instanceof FileHandle, 'FileHandle is returned')
  await fd.close()
})

test('fs.promises.opendir', async (t) => {
  const dir = await fs.opendir(FIXTURES + 'directory')
  t.ok(dir instanceof Dir, 'fs.Dir is returned')
  await dir.close()
})

test('fs.promises.readdir', async (t) => {
  const files = await fs.readdir(FIXTURES + 'directory')
  t.ok(Array.isArray(files), 'array is returned')
  t.equal(files.length, 6, 'array contains 2 items')
  t.deepEqual(files.map(file => file.name), ['0', '1', '2', 'a', 'b', 'c'].map(name => `${name}.txt`), 'array contains files')
})

test('fs.promises.readFile', async (t) => {
  const data = await fs.readFile(FIXTURES + 'file.txt')
  t.ok(Buffer.isBuffer(data), 'buffer is returned')
  t.equal(data.toString(), 'test 123\n', 'buffer contains file contents')
})

test('fs.promises.stat', async (t) => {
  let stats = await fs.stat(FIXTURES + 'file.txt')
  t.ok(stats, 'stats are returned')
  t.equal(stats.isFile(), true, 'stats are for a file')
  t.equal(stats.isDirectory(), false, 'stats are not for a directory')
  t.equal(stats.isSymbolicLink(), false, 'stats are not for a symbolic link')
  t.equal(stats.isSocket(), false, 'stats are not for a socket')
  t.equal(stats.isFIFO(), false, 'stats are not for a FIFO')
  t.equal(stats.isBlockDevice(), false, 'stats are not for a block device')
  t.equal(stats.isCharacterDevice(), false, 'stats are not for a character device')

  stats = await fs.stat(FIXTURES + 'directory')
  t.ok(stats, 'stats are returned')
  t.equal(stats.isFile(), false, 'stats are not for a file')
  t.equal(stats.isDirectory(), true, 'stats are for a directory')
  t.equal(stats.isSymbolicLink(), false, 'stats are not for a symbolic link')
  t.equal(stats.isSocket(), false, 'stats are not for a socket')
  t.equal(stats.isFIFO(), false, 'stats are not for a FIFO')
  t.equal(stats.isBlockDevice(), false, 'stats are not for a block device')
  t.equal(stats.isCharacterDevice(), false, 'stats are not for a character device')
})

test('fs.promises.writeFile', async (t) => {
  const file = FIXTURES + 'write-file.txt'
  const data = 'test 123\n'
  await fs.writeFile(file, data)
  const contents = await fs.readFile(file)
  t.equal(contents.toString(), data, 'file contents are correct')
})
