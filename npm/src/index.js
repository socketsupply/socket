import path from 'node:path'
import os from 'node:os'
import fs from 'node:fs'
import { spawn } from 'node:child_process'

const dirname = path
  .dirname(import.meta.url)
  .replace(`file://${os.platform() === 'win32' ? '/' : ''}`, '')

export const SOCKET_HOME = path.dirname(dirname)
export const PREFIX = SOCKET_HOME

export const platform = os.platform()
export const arch = os.arch()

export const env = {
  SOCKET_HOME,
  PREFIX
}

export const bin = {
  'ssc-platform': path.join(env.PREFIX, 'bin', 'ssc-platform.js'),
  ssc: os.platform() === 'win32'
    ? path.join(env.PREFIX, 'bin', 'ssc.exe')
    : path.join(env.PREFIX, 'bin', 'ssc')
}

// Modifying this function requires full FTE testing (From fresh install) on all build OS's before signing off (Individual arch testing not required)
export const firstTimeExperienceSetup = async () => {
  const installPath = path.dirname(path.dirname(bin.ssc))
  if (fs.existsSync(path.join(installPath, '.ssc.env'))) {
    return true
  }

  // ssc env doesn't exist, attempt to run setup for target being built.
  // This should also install vc_redist if it hasn't been installed
  const PLATFORM_PARAMETER = '--platform='
  let isSetupCall = false

  let platform = ''
  process.argv.slice(2).forEach(arg => {
    if (arg === 'setup') {
      isSetupCall = true
    }

    if (arg.indexOf(PLATFORM_PARAMETER) === 0) {
      platform = arg.slice(PLATFORM_PARAMETER.length)
    }
  })

  if (platform === 'android-emulator') {
    platform = 'android'
  }

  const startInfo = {
    env: { ...process.env },
    cwd: installPath,
    stdio: [process.stdin, process.stdout, process.stderr]
  }

  const spawnArgs = []
  if (os.platform() === 'win32') {
    spawnArgs.push(
      // @ts-ignore
      'powershell.exe',
      ['.\\bin\\install.ps1', `-fte:${platform.length > 0 ? platform : 'all'}`],
      startInfo
    )
  } else {
    spawnArgs.push(
      // @ts-ignore
      './bin/functions.sh',
      ['--fte', platform],
      startInfo
    )
  }

  if (spawnArgs.length) {
    if (isSetupCall) {
      startInfo.env.VERBOSE = 1
      console.log('# Checking build dependencies...')
    }

    // @ts-ignore
    const child = spawn(...spawnArgs)

    await new Promise((resolve, reject) => {
      // @ts-ignore
      child.on('close', resolve).on('error', reject)
    })

    // If fte didn't create a configuration file, make an empty one to prevent
    // user being prompted again
    // @ts-ignore
    if (!child.exitCode && !fs.existsSync(path.join(installPath, '.ssc.env'))) {
      fs.writeFileSync(path.join(installPath, '.ssc.env'), '')
    }
  } else {
    return true
  }

  return !isSetupCall
}

export default {
  platform,
  arch,
  env,
  bin,
  firstTimeExperienceSetup
}
