#!/usr/bin/env node

import installation from '../src/index.js'
import { spawn } from 'node:child_process'

export default spawn(installation.bin.ssc, process.argv.slice(2), {
  env: { ...installation.env, ...process.env },
  stdio: 'inherit',
  windowsHide: true
})
