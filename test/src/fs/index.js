import { Buffer } from 'socket:buffer'
import deepEqual from 'socket:test/fast-deep-equal'
import { test } from 'socket:test'
import crypto from 'socket:crypto'
import path from 'socket:path'
import fs from 'socket:fs'
import os from 'socket:os'
import process from 'socket:process'

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

if (os.platform() !== 'android' && os.platform() !== 'win32') {
  test.skip('fs.chown', async (t) => {
    // TODO: implement process.getuid and process.getgid @bcomnes
    await new Promise((resolve, reject) => {
      const uid = process.getuid()
      const gid = process.getgid()
      fs.chown(FIXTURES + 'chown.txt', uid, gid, (err) => {
        if (err) t.fail(err)
        fs.stat(FIXTURES + 'file.txt', (err, stats) => {
          if (err) t.fail(err)
          t.equal(stats.uid, uid, 'the uid matches the stat')
          t.equal(stats.gid, gid, 'the gid matches the stat')
          resolve()
        })
      })
    })
  })
}

test('fs.open + fs.close', async (t) => {
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

test('fs.copyFile', async (t) => {
  await new Promise((resolve, reject) => {
    const src = path.join(FIXTURES, 'file.txt')
    const dest = path.join(FIXTURES, 'copy.txt')

    fs.copyFile(src, dest, 0, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }
      t.pass('File was copied without error')

      fs.stat(dest, (err, stats) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.ok(stats, 'Copied file was stated without error')

        fs.readFile(src, 'utf8', (err, srcData) => {
          if (err) {
            t.fail(err)
            return resolve()
          }

          t.ok(srcData, 'The src data was read without error')

          fs.readFile(dest, 'utf8', (err, destData) => {
            if (err) {
              t.fail(err)
              return resolve()
            }

            t.ok(srcData, 'The copied data was read without error')

            t.equal(destData, srcData, 'the copy contains a copy of the data')

            fs.unlink(dest, (err) => {
              if (err) {
                t.fail(err)
                return resolve()
              }
              t.ok('The copied file is removed')
              return resolve()
            })
          })
        })
      })
    })
  })
})

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

test.skip('fs.fstat', async (t) => {
  // TODO: fix fstat @bcomnes @jwerle
  await new Promise((resolve, reject) => {
    fs.open(path.join(FIXTURES, 'file.txt'), (err, fd) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.ok(Number.isFinite(fd), 'isFinite(fd)')

      fs.fstat(fd, (err, stats, ...rest) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.ok(stats, 'the stats object is included')

        fs.close(fd, (err) => {
          if (err) t.fail(err)

          t.ok(!err, 'fd closed')
          resolve()
        })
      })
    })
  })
})

// TODO
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
}

if (os.platform() !== 'android') {
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

test('fs.opendir', async (t) => {
  await new Promise((resolve, reject) => {
    fs.opendir(FIXTURES, async (err, dir) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      try {
        t.ok(!dir.closed, 'the dirent is open before iterating')
        let count = 0
        for await (const dirent of dir) {
          t.ok(dirent.name, `loop through dirents and they have names: ${dirent.name}`)
          count++
        }
        t.ok(count > 0, 'There are more than 0 dirents')
        t.ok(dir.closed, 'the dirent is closed after iterating')
        return resolve()
      } catch (err) {
        t.fail(err)
        return resolve()
      }
    })
  })
})

test('fs.read', async (t) => {
  await new Promise((resolve, reject) => {
    fs.open(FIXTURES + 'file.txt', (err, fd) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.ok(Number.isFinite(fd), 'isFinite(fd)')
      const readLength = 8
      const readBuff = new Int8Array(readLength)

      fs.read(fd, readBuff, 0, readLength, 0, (err, bytesRead, buffer) => {
        if (err) {
          t.fail(err)
          return resolve()
        }
        const expected = 'test 123'
        const returnedBuf = new TextDecoder().decode(buffer)
        const targetBuf = new TextDecoder().decode(readBuff)

        t.equal(returnedBuf, expected, 'returned buffer has the correct data')
        t.equal(targetBuf, expected, 'target buffer has the correct data')

        fs.close(fd, (err) => {
          if (err) t.fail(err)

          t.ok(!err, 'fd closed')
          resolve()
        })
      })
    })
  })
})

test('fs.readdir', async (t) => {
  await new Promise((resolve, reject) => {
    fs.readdir(FIXTURES, async (err, files) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.ok(files.length >= 7, 'has the correct number of files in it')
      ;[
        'bin',
        'chown.txt',
        'data.bin',
        'directory',
        'file.js',
        'file.json',
        'file.txt'
      ].forEach(file => {
        t.ok(files.includes(file), `includes ${file}`)
      })

      return resolve()
    })
  })
})

test('fs.readdir withFileTypes', async (t) => {
  await new Promise((resolve, reject) => {
    fs.readdir(FIXTURES, { withFileTypes: true }, async (err, files) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.ok(files.length >= 7, 'has the correct number of files in it')

      t.ok(files.every(dirent => dirent instanceof fs.Dirent), 'every dirent in the files array is actually a dirent')
      return resolve()
    })
  })
})

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

// TODO: ensure this is working as expected. Its not working like node @bcomnes
// resolving to "/Users/userHomeDir/socket/test/fixtures/file.txt" on macos
test('fs.readlink', async (t) => {
  await new Promise((resolve, reject) => {
    const link = path.join(FIXTURES, 'link.txt')
    fs.readlink(link, (_, resolvedPath) => {
      t.ok(resolvedPath.endsWith('/file.txt'), 'link path matches the actual path')
      return resolve()
    })
  })
})

// TODO: ensure this is working as expected. Its not working like node @bcomnes
test('fs.realpath', async (t) => {
  await new Promise((resolve, reject) => {
    const link = path.join(FIXTURES, 'link.txt')
    fs.realpath(link, (_, resolvedPath) => {
      t.ok(resolvedPath.endsWith('/file.txt'), 'link path matches the actual path')
      return resolve()
    })
  })
})

test('fs.rename', async (t) => {
  await new Promise((resolve, reject) => {
    const src = path.join(FIXTURES, 'file.txt')
    const dest = path.join(FIXTURES, 'rename.txt')
    fs.rename(src, dest, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.pass('File was renamed without error')

      fs.stat(dest, (err, stats) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.ok(stats, 'Renamed file was stated without error')

        fs.rename(dest, src, (err) => {
          if (err) {
            t.fail(err)
            return resolve()
          }

          fs.stat(src, (err, stats) => {
            if (err) {
              t.fail(err)
              return resolve()
            }

            t.ok(stats, 'Renamed file was moved back to the original location')
            return resolve()
          })
        })
      })
    })
  })
})

if (os.platform() !== 'android') {
  test('fs.rmdir', async (t) => {
    await new Promise((resolve, reject) => {
      const target = path.join(FIXTURES, 'rmdir-dir')
      fs.mkdir(target, { recursive: true }, (err) => {
        if (err) {
          t.fail(err)
          return resolve()
        }
        t.pass('The directory is created without error')
        fs.rmdir(target, (err) => {
          if (err) {
            t.fail(err)
            return resolve()
          }
          t.pass('The directory is removed without error')
          fs.stat(target, (err, stats) => {
            if (err) {
              t.pass('The directory is removed and no longer stats')
              return resolve()
            } else {
              t.fail('The directory should fail to stat')
              return resolve()
            }
          })
        })
      })
    })
  })
}

test('fs.stat', async (t) => {
  await new Promise((resolve, reject) => {
    const target = path.join(FIXTURES, 'file.txt')
    fs.stat(target, (err, stats) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.ok(stats, 'stat object is returned')
      return resolve()
    })
  })
})

test('fs.symlink', async (t) => {
  await new Promise((resolve, reject) => {
    const src = path.join(FIXTURES, 'file.txt')
    const dest = path.join(FIXTURES, 'symlink.txt')
    fs.symlink(src, dest, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.pass('The symlink is made without error')

      // TODO: Update this when realpath is fixed
      fs.realpath(dest, (resolvedPath) => {
        t.ok(resolvedPath.endsWith('/file.txt'), 'link path matches the actual path')

        fs.unlink(dest, (err) => {
          if (err) {
            t.fail(err)
            return resolve()
          }
          t.ok('The synlink is removed')
          return resolve()
        })
      })
    })
  })
})

// This doesn't work yet
test.skip('fs.ftruncate', async (t) => {
  await new Promise((resolve, reject) => {
    const testString = 'test 123 test 123 test 123 test 123 test 123'
    const buffer = Buffer.from(testString)
    const filePath = path.join(TMPDIR, 'truncate.txt')
    fs.writeFile(filePath, buffer, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.pass('The test file is created')

      fs.open(filePath, (err, fd) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.ok(fd, 'The test file is opened and we have an fd')

        fs.ftruncate(fd, 5, (err) => {
          if (err) {
            console.error({ err })
            t.fail(err)
            return resolve()
          }

          t.pass('The test file is truncated')

          fs.readFile(filePath, (err, result) => {
            if (err) {
              t.fail(err)
              return resolve()
            }

            t.equal(result, testString)

            fs.close(fd, (err) => {
              if (err) t.fail(err)

              t.ok(!err, 'fd closed')
              resolve()
            })
          })
        })
      })
    })
  })
})

test('fs.unlink', async (t) => {
  const buffer = Buffer.from('test 123')
  await new Promise((resolve, reject) => {
    const filePatht = path.join(TMPDIR, 'new-file.txt')
    fs.writeFile(filePatht, buffer, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.pass('The file is written wuthout error')

      fs.readFile(filePatht, (err, result) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.pass('The file is written wuthout error')

        t.equal(Buffer.compare(result, buffer), 0, 'bytes match')

        fs.unlink(filePatht, (err) => {
          if (err) {
            t.fail(err)
            return resolve()
          }
          t.ok('The test file is removed without error')

          fs.stat(filePatht, (err, stats) => {
            if (err) {
              t.pass('The file is removed and no longer stats')
              return resolve()
            } else {
              t.fail('The file should fail to stat')
              return resolve()
            }
          })
        })
      })
    })
  })
})

// Not working yet
test.skip('fs.watch', async (t) => {
  let watcherFired = false
  const watcher = fs.watch(TMPDIR, (...eventArgs) => {
    watcherFired = true
  })
  await watcher.start()
  const buffer = Buffer.from('test 123')

  await new Promise((resolve, reject) => {
    fs.writeFile(TMPDIR + 'watch-file1.txt', buffer, (err) => {
      if (err) {
        t.fail(err)
        return resolve()
      }

      t.pass('watch-file1.txt written')

      fs.writeFile(TMPDIR + 'watch-file2.txt', buffer, (err) => {
        if (err) {
          t.fail(err)
          return resolve()
        }

        t.pass('watch-file2.txt written')
        fs.writeFile(TMPDIR + 'watch-file3.txt', buffer, (err) => {
          if (err) {
            t.fail(err)
            return resolve()
          }
          t.pass('watch-file3.txt written')
          resolve()
        })
      })
    })
  })

  await watcher.close()
  t.ok(watcherFired, 'The watcher should have fired at least once')
})

if (os.platform() !== 'android') {
  test('fs.writeFile', async (t) => {
    const buffer = Buffer.from('test 123')
    await new Promise((resolve, reject) => {
      fs.writeFile(TMPDIR + 'new-file.txt', buffer, (err) => {
        if (err) t.fail(err.message)

        fs.readFile(TMPDIR + 'new-file.txt', (err, result) => {
          if (err) t.fail(err.message)
          else if (Buffer.compare(result, buffer) !== 0) t.fail('bytes do not match')
          resolve()
        })
      })
    })

    // TODO: move the code below to a benchmark tests

    // const alloc = (size) => crypto.randomBytes(size)
    // const small = Array.from({ length: 32 }, (_, i) => i * 2 * 1024).map(alloc)

    // const large = Array.from({ length: 16 }, (_, i) => i * 2 * 1024 * 1024).map(alloc)
    // const buffers = [...small, ...large]

    // // const pending = buffers.length
    // let failed = false
    // const writes = []

    // // const now = Date.now()
    // while (!failed && buffers.length) {
    //   writes.push(testWrite(buffers.length - 1, buffers.pop()))
    // }

    // await Promise.all(writes)

    // // console.log(
    // //   '%d writes to %sms to write %s bytes',
    // //   small.length + large.length,
    // //   Date.now() - now,
    // //   [...small, ...large].reduce((n, a) => n + a.length, 0)
    // // )

    // t.ok(!failed, 'all bytes match')

    // async function testWrite (i, buffer) {
    //   await new Promise((resolve) => {
    //     const filename = TMPDIR + `new-file-${i}.txt`
    //     fs.writeFile(filename, buffer, async (err) => {
    //       if (err) {
    //         failed = true
    //         t.fail(err.message)
    //         return resolve()
    //       }

    //       fs.readFile(filename, (err, result) => {
    //         if (err) {
    //           failed = true
    //           t.fail(err.message)
    //         } else if (Buffer.compare(result, buffer) !== 0) {
    //           failed = true
    //           t.fail('bytes do not match')
    //         }

    //         resolve()
    //       })
    //     })
    //   })
    // }
  })
}
