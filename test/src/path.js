import path from '../../api/path.js'
import process from '../../api/process.js'
import os from '../../api/os.js'

import { test } from '@socketsupply/tapzero'

test('path', (t) => {
  const isUnix = os.platform() !== 'win32'
  t.ok(path.posix, 'path.posix exports')
  t.ok(path.win32, 'path.win32 exports')
  const expectedSep = isUnix ? '/' : '\\'
  t.equal(path.sep, expectedSep, 'path.sep is correct')
  const expectedDelimiter = isUnix ? ':' : ';'
  t.equal(path.delimiter, expectedDelimiter, 'path.delimiter is correct')
  const cwd = isUnix ? process.cwd() : process.cwd().replace(/\\/g, '/')
  t.equal(path.cwd(), cwd, 'path.cwd() returns the current working directory')
})
