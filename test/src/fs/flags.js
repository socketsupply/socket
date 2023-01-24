import { normalizeFlags } from '../../../fs/flags.js'
import { test } from '@socketsupply/tapzero'
import * as fs from '../../../fs.js'

test('flags', (t) => {
  t.ok(
    normalizeFlags() === fs.constants.O_RDONLY,
    'undefined === fs.constants.O_RDONLY'
  )

  t.ok(
    normalizeFlags(fs.constants.O_WRONLY) === fs.constants.O_WRONLY,
    'fs.constants.O_WRONLY=== fs.constants.O_WRONLY'
  )

  t.throws(
    () => normalizeFlags(null),
    // eslint-disable-next-line prefer-regex-literals
    RegExp('Expecting flags to be a string or number: Got object'),
    'normalizeFlags() throws on null'
  )

  t.throws(() =>
    normalizeFlags({}),
  // eslint-disable-next-line prefer-regex-literals
  RegExp('Expecting flags to be a string or number: Got object'),
  'normalizeFlags() throws on object'
  )

  t.throws(
    () => normalizeFlags(true),
    // eslint-disable-next-line prefer-regex-literals
    RegExp('Expecting flags to be a string or number: Got boolean'),
    'normalizeFlags() throws on boolean'
  )

  t.ok(
    normalizeFlags('r') === fs.constants.O_RDONLY,
    'r === fs.constants.O_RDONLY'
  )

  t.ok(
    normalizeFlags('rs') === fs.constants.O_RDONLY | fs.constants.O_SYNC,
    'rs === fs.constants.O_RDONLY | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('sr') === fs.constants.O_RDONLY | fs.constants.O_SYNC, 'sr === fs.constants.O_RDONLY | fs.constants.O_SYNC')

  t.ok(
    normalizeFlags('r+') === fs.constants.O_RDWR,
    'r+ === fs.constants.O_RDWR'
  )

  t.ok(
    normalizeFlags('rs+') === fs.constants.O_RDWR | fs.constants.O_SYNC,
    'rs+ === fs.constants.O_RDWR | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('sr+') === fs.constants.O_RDWR | fs.constants.O_SYNC,
    'sr+ === fs.constants.O_RDWR | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('w') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY,
    'w === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY'
  )

  t.ok(
    normalizeFlags('wx') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL,
    'wx === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('xw') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL,
    'xw === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('w+') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR,
    'w+ === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR'
  )

  t.ok(
    normalizeFlags('wx+') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL,
    'wx+ === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL'
  )
  t.ok(
    normalizeFlags('xw+') === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL, 'xw+ === fs.constants.O_TRUNC | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL')

  t.ok(
    normalizeFlags('a') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY,
    'a === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY'
  )

  t.ok(
    normalizeFlags('ax') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL,
    'ax === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('xa') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL,
    'xa === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('as') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_SYNC,
    'as === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('sa') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_SYNC,
    'sa === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_WRONLY | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('a+') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR,
    'a+ === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR'
  )

  t.ok(
    normalizeFlags('ax+') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL,
    'ax+ === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('xa+') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL,
    'xa+ === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_EXCL'
  )

  t.ok(
    normalizeFlags('as+') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_SYNC,
    'as+ === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_SYNC'
  )

  t.ok(
    normalizeFlags('sa+') === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_SYNC,
    'sa+ === fs.constants.O_APPEND | fs.constants.O_CREAT | fs.constants.O_RDWR | fs.constants.O_SYNC'
  )
})
