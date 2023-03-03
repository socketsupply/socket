#!/usr/bin/env node

import { fork } from 'node:child_process'
import path from 'node:path'
import os from 'node:os'

const dirname = path.dirname(import.meta.url).replace(`file://${os.platform() === 'win32' ? '/' : ''}`, '')

let exiting = false

export async function load () {
  const platform = os.platform()
  const arch = os.arch()
  const info = await import(`@socketsupply/socket-${platform}-${arch}`)
  return info?.default ?? info ?? null
}

async function main () {
  const installation = await load()
  const env = {
    ...installation.env,
    SOCKET_HOME_API: path.dirname(dirname),
    ...process.env
  }

  const child = fork(installation.bin['ssc-platform'], process.argv.slice(2), {
    env,
    stdio: 'inherit',
    windowsHide: true
  })

  child.once('exit', (code) => {
    if (!exiting) {
      exiting = true
      process.exit(code)
    }
  })

  child.once('error', (err) => {
    console.error(err.message)
    if (!exiting) {
      process.exit(1)
      exiting = true
    }
  })
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
