#!/usr/bin/env node

import { execSync as exec } from 'node:child_process'

async function main () {
  exec('cd ps1 && powershell .\\install.ps1 -shbuild:$false', { stdio: 'inherit' })
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
