import { rmSync as rm, cpSync as cp } from 'node:fs'
import { execSync as exec } from 'node:child_process'
import path from 'node:path'
import os from 'node:os'

const dirname = path.dirname(import.meta.url.replace('file://', '').replace(/^\/[A-Za-z]:/, ''))
const root = path.dirname(dirname)

const SOCKET_HOME_API = path.join(root, '..', 'api')
const {
  DEBUG,
  TMP,
  TMPDIR = TMP || os.tmpdir()
} = process.env

try {
  rm(path.join(TMPDIR, 'ssc-socket-test-fixtures'), {
    recursive: true,
    force: true
  })
} catch {}

cp(path.join(root, 'fixtures'), path.join(TMPDIR, 'ssc-socket-test-fixtures'), {
  recursive: true
})

try {
  exec(`ssc build ${!DEBUG ? '--headless --prod' : ''} -r -o`, {
    stdio: 'inherit',
    env: {
      SOCKET_HOME_API,
      ...process.env
    }
  })
} catch (err) {
  process.exit(err.status || 1)
}
