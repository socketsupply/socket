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
  const sscCmdPath = path.join(process.cwd(), '../../.bin/ssc.cmd')

  if (!fs.existsSync(sscCmdPath)) {
    console.log(`WARNING: No ${sscCmdPath} builds will fail - please report (https://discord.gg/YPV32gKCsH)`)
    return
  }
  let sscCmdData = fs.readFileSync(sscCmdPath, 'utf-8')

  let envs = ''
  let updated = false
  for (const e in env) {
    const setLine = `SET ${e}=${env[e]}\r\n`
    envs += setLine
    // only insert or update variable if it isn't there already
    if (sscCmdData.indexOf(setLine) === -1) {
      if (sscCmdData.indexOf(`${e}=`) > -1) {
        // replace if variable exists
        sscCmdData = sscCmdData.replace(new RegExp(`${e}=.*`), `${e}=${env[e]}`)
        updated = true
      } else {
        // insert variable after @ECHO off
        sscCmdData = sscCmdData.replace('@ECHO off\r\n', `@ECHO off\r\n${setLine}`)
        updated = true
      }
    }
  }

  fs.writeFileSync('env.bat', envs)
  if (updated === true) { fs.writeFileSync(sscCmdPath, sscCmdData) }
}

async function preinstall () {
  const cmdProcess = spawn(
    'powershell.exe',
    ['.\\install.ps1', '-shbuild:$false', '-package_setup:$true'],
    { cwd: 'ps1', stdio: [process.stdin, process.stdout, process.stderr] }
  )

  await new Promise((resolve, reject) => {
    cmdProcess.on('close', resolve).on('error', reject)
  })
}

async function main (argv) {
  if (argv.length === 0) { return }

  if (argv[0] === 'preinstall') { await preinstall() }
  if (argv[0] === 'postinstall') { await insertEnvVars() }
  if (argv[0] === 'install') {
    await preinstall()
    await insertEnvVars()
  }
}

main(process.argv.slice(2)).catch((err) => {
  console.error(err)
  process.exit(1)
})
