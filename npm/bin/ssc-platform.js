#!/usr/bin/env node

import installation from '../src/index.js'
import { spawn } from 'node:child_process'

let exiting = false

const child = spawn(installation.bin.ssc, process.argv.slice(2), {
  env: { ...installation.env, ...process.env },
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

export default child
