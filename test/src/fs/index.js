import { Buffer } from 'socket:buffer'
import deepEqual from 'socket:test/fast-deep-equal'
import { test } from 'socket:test'
import crypto from 'socket:crypto'
import path from 'socket:path'
import fs from 'socket:fs'
import os from 'socket:os'
import process from 'socket:process'

// node compat
/*
import crypto from 'node:crypto'
import fs from 'node:fs'
import os from 'node:os'
*/

// FIXME: make this work on iOS
if (process.platform !== 'ios') {
  const TMPDIR = `${os.tmpdir()}${path.sep}`
  const FIXTURES = /android/i.test(os.platform())
    ? '/data/local/tmp/ssc-socket-test-fixtures/'
    : `${TMPDIR}ssc-socket-test-fixtures${path.sep}`

  test('fs.access', async (t) => {
    const { F_OK, R_OK, W_OK, X_OK } = fs.constants

    await new Promise((resolve, reject) => {
      fs.access(FIXTURES, F_OK, (err, access) => {
        if (err) t.fail(`(F_OK) ${FIXTURES} is not accessible`)
        else t.ok(access, '(F_OK) fixtures/ directory is accessible')
        resolve()
      })
    })

    await new Promise((resolve, reject) => {
      fs.access(FIXTURES, R_OK, (err, access) => {
        if (err) t.fail(`(R_OK) ${FIXTURES} is not readable`)
        else t.ok(access, '(R_OK) fixtures/ directory is readable')
        resolve()
      })
    })

    await new Promise((resolve, reject) => {
      fs.access('.', W_OK, (err, access) => {
        if (err) t.fail('(W_OK) ./ is not writable')
        else t.ok(access, '(W_OK) ./ directory is writable')
        resolve()
      })
    })

    await new Promise((resolve, reject) => {
      fs.access(FIXTURES, X_OK, (err, access) => {
        if (err) t.fail(`(X_OK) ${FIXTURES} directory is not "executable" - cannot list items`)
        else t.ok(access, '(X_OK) fixtures/ directory is "executable" - can list items')
        resolve()
      })
    })
  })

  test('fs.appendFile', async (t) => {})

  if (os.platform() !== 'android' && os.platform() !== 'win32') {
    test('fs.chmod', async (t) => {
      await new Promise((resolve, reject) => {
        fs.chmod(FIXTURES + 'file.txt', 0o777, (err) => {
          if (err) t.fail(err)
          fs.stat(FIXTURES + 'file.txt', (err, stats) => {
            if (err) t.fail(err)
            t.equal(stats.mode & 0o777, 0o777, 'file.txt mode is 777')
            resolve()
          })
        })
      })
    })
  }

  test('fs.chown', async (t) => {})

  test('fs.close', async (t) => {
    await new Promise((resolve, reject) => {
      fs.open(FIXTURES + 'file.txt', (err, fd) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.ok(Number.isFinite(fd), 'isFinite(fd)')
        fs.close(fd, (err) => {
          if (err) t.fail(err)

          t.ok(!err, 'fd closed')
          resolve()
        })
      })
    })
  })

  test('fs.copyFile', async (t) => {})

  test('fs.createReadStream', async (t) => {
    if (os.platform() === 'android') {
      t.comment('FIXME for Android')
      return
    }

    const buffers = []
    await new Promise((resolve, reject) => {
      const stream = fs.createReadStream(FIXTURES + 'file.txt')
      const expected = Buffer.from('test 123')

      stream.on('close', resolve)
      stream.on('data', (buffer) => {
        buffers.push(buffer)
      })

      stream.on('error', (err) => {
        if (err) t.fail(err)
        resolve()
      })

      stream.on('end', () => {
        let actual = Buffer.concat(buffers)
        if (actual[actual.length - 1] === 0x0A) {
          actual = actual.slice(0, -1)
        }

        if (actual[actual.length - 1] === 0x0D) {
          actual = actual.slice(0, -1)
        }

        t.ok(
          Buffer.compare(expected, actual) === 0,
          `fixtures/file.txt contents match "${expected}"`
        )
      })
    })
  })

  test('fs.createWriteStream', async (t) => {
    if (os.platform() === 'android') return t.comment('TODO')
    const writer = fs.createWriteStream(TMPDIR + 'new-file.txt')
    const bytes = crypto.randomBytes(32 * 1024 * 1024)
    writer.write(bytes.slice(0, 512 * 1024))
    writer.write(bytes.slice(512 * 1024))
    writer.end()
    await new Promise((resolve) => {
      writer.once('error', (err) => {
        t.fail(err.message)
        writer.removeAllListeners()
        resolve()
      })
      writer.once('close', () => {
        const reader = fs.createReadStream(TMPDIR + 'new-file.txt')
        const buffers = []
        reader.on('data', (buffer) => buffers.push(buffer))
        reader.on('end', () => {
          t.ok(Buffer.compare(bytes, Buffer.concat(buffers)) === 0, 'bytes match')
          resolve()
        })
      })
    })
  })

  test('fs.fstat', async (t) => {})
  test('fs.lchmod', async (t) => {})
  test('fs.lchown', async (t) => {})
  test('fs.lutimes', async (t) => {})
  test('fs.link', async (t) => {})
  test('fs.lstat', async (t) => {})
  if (os.platform() !== 'android') {
    test('fs.mkdir', async (t) => {
      const dirname = FIXTURES + Math.random().toString(16).slice(2)
      await new Promise((resolve, reject) => {
        fs.mkdir(dirname, {}, (err) => {
          if (err) reject(err)

          fs.stat(dirname, (err) => {
            if (err) reject(err)
            resolve()
          })
        })
      })
    })

    test('fs.mkdir recursive', async (t) => {
      const randomDirSegment = () => Math.random().toString(16).slice(2)
      const dirname = path.join(FIXTURES, randomDirSegment(), randomDirSegment(), randomDirSegment())
      await new Promise((resolve, reject) => {
        fs.mkdir(dirname, { recursive: true }, (err) => {
          if (err) reject(err)

          fs.stat(dirname, (err) => {
            if (err) reject(err)
            resolve()
          })
        })
      })
    })
  }
  test('fs.open', async (t) => {})
  test('fs.opendir', async (t) => {})
  test('fs.read', async (t) => {})
  test('fs.readdir', async (t) => {})
  test('fs.readFile', async (t) => {
    let failed = false
    const iterations = 16 // generate ~1k _concurrent_ requests
    const expected = { data: 'test 123' }
    const promises = Array.from(Array(iterations), (_, i) => new Promise((resolve) => {
      if (failed) return resolve(false)
      fs.readFile(FIXTURES + 'file.json', (err, buf) => {
        if (failed) return resolve(false)

        const message = `fs.readFile('fixtures/file.json') [iteration=${i + 1}]`

        try {
          if (err) {
            t.fail(err, message)
            failed = true
          } else if (!deepEqual(expected, JSON.parse(buf))) {
            failed = true
          }
        } catch (err) {
          t.fail(err, message)
          failed = true
        }

        resolve(!failed)
      })
    }))

    const results = await Promise.all(promises)
    t.ok(results.every(Boolean), 'fs.readFile(\'fixtures/file.json\')')
  })

  test('fs.readlink', async (t) => {})
  test('fs.realpath', async (t) => {})
  test('fs.rename', async (t) => {})
  test('fs.rmdir', async (t) => {})
  test('fs.rm', async (t) => {})
  test('fs.stat', async (t) => {})
  test('fs.symlink', async (t) => {})
  test('fs.truncate', async (t) => {})
  test('fs.unlink', async (t) => {})
  test('fs.utimes', async (t) => {})
  test('fs.watch', async (t) => {})
  test('fs.write', async (t) => {})

  if (os.platform() !== 'android') {
    test('fs.writeFile', async (t) => {
      const alloc = (size) => crypto.randomBytes(size)
      const small = Array.from({ length: 32 }, (_, i) => i * 2 * 1024).map(alloc)
      const large = Array.from({ length: 16 }, (_, i) => i * 2 * 1024 * 1024).map(alloc)
      const buffers = [...small, ...large]

      // const pending = buffers.length
      let failed = false
      const writes = []

      // const now = Date.now()
      while (!failed && buffers.length) {
        writes.push(testWrite(buffers.length - 1, buffers.pop()))
      }

      await Promise.all(writes)
      /*
    console.log(
      '%d writes to %sms to write %s bytes',
      small.length + large.length,
      Date.now() - now,
      [...small, ...large].reduce((n, a) => n + a.length, 0)
    ) */

      t.ok(!failed, 'all bytes match')

      async function testWrite (i, buffer) {
        await new Promise((resolve) => {
          const filename = TMPDIR + `new-file-${i}.txt`
          fs.writeFile(filename, buffer, async (err) => {
            if (err) {
              failed = true
              t.fail(err.message)
              return resolve()
            }

            fs.readFile(filename, (err, result) => {
              if (err) {
                failed = true
                t.fail(err.message)
              } else if (Buffer.compare(result, buffer) !== 0) {
                failed = true
                t.fail('bytes do not match')
              }

              resolve()
            })
          })
        })
      }
    })
  }

  test('fs.writev', async (t) => {})
}
