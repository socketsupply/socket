import test from 'socket:test'
import path from 'socket:path'
import fs from 'socket:fs/promises'
import os from 'socket:os'

if (!['android'].includes(os.platform())) {
  test('src/copy-map/file-a.js -> copy-map/A.js', async (t) => {
    t.ok(await fs.access(path.join('copy-map', 'A.js')), 'A.js exists')

    try {
      await fs.access(path.join('copy-map', 'file-a.js'))
    } catch (err) {
      t.ok(/no such file/i.test(err?.message), 'file-a.js does not exist')
    }
  })

  test('src/copy-map/file-b.js -> copy-map/B.js', async (t) => {
    t.ok(await fs.access(path.join('copy-map', 'B.js')), 'B.js exists')
    try {
      await fs.access(path.join('copy-map', 'file-b.js'))
    } catch (err) {
      t.ok(/no such file/i.test(err?.message), 'file-b.js does not exist')
    }
  })
}
