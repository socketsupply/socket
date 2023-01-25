#!/usr/bin/env node

import { fork } from 'node:child_process'
import { load } from '../index.js'

async function main () {
  const installation = await load()
  fork(installation.bin['ssc-platform'], process.argv.slice(2), {
    env: { ...installation.env, ...process.env },
    stdio: 'inherit',
    windowsHide: true
  })
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
