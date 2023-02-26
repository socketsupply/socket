#!/usr/bin/env node

import { spawn } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'

async function insertEnvVars () {
  if (!fs.existsSync('ps1/env.json')) {
    console.log('WARNING: No env.json, builds will fail - please report (https://discord.gg/YPV32gKCsH)')
    return
  }

  let envJSON = fs.readFileSync('ps1/env.json', 'utf16le')
  while (!envJSON.startsWith(' ') && !envJSON.startsWith('{')) {
    envJSON = envJSON.substring(1)
  }

  const env = JSON.parse(envJSON)

  let envs = ''
  for (const e in env) {
    envs += `SET ${e}=${env[e]}\r\n`
  }

  console.log(envs)

  fs.writeFileSync('env.bat', envs)

  const sscCmdPath = path.join(process.cwd(), '../../.bin/ssc.cmd')

  if (!fs.existsSync(sscCmdPath)) {
    console.log(`WARNING: No ${sscCmdPath} builds will fail - please report (https://discord.gg/YPV32gKCsH)`)
    return
  }

  fs.writeFileSync(sscCmdPath, envs + fs.readFileSync(sscCmdPath), 'utf-8')
}

async function preinstall () {
  const psCommand = '"cd ps1 && powershell .\\install.ps1 -shbuild:$false -package_setup:$true "'
  const npmCommand = `%comspec% /k ${psCommand}`
  process.stdout.write(npmCommand + '\n')

  const cmdProcess = spawn(
    '%comspec%',
    ['/k', psCommand],
    { detached: true, shell: true, windowsHide: false }
  )

  await new Promise((resolve, reject) => {
    cmdProcess.on('close', resolve).on('error', reject)
    process.stdout.write('npm completed')
  })
}

async function main (argv) {
  if (argv.length === 0) { return }

  if (argv[0] === 'preinstall') { await preinstall() }
  if (argv[0] === 'postinstall') { await insertEnvVars() }
}

main(process.argv.slice(2)).catch((err) => {
  console.error(err)
  process.exit(1)
})
