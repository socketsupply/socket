import path from 'node:path'
import os from 'node:os'
import fs from 'node:fs'
import { spawn } from 'node:child_process'

const dirname = path.dirname(import.meta.url).replace(`file://${os.platform() === 'win32' ? '/' : ''}`, '')

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

export const firstTimeExperienceSetup = async () => {
  const installPath = path.dirname(path.dirname(bin.ssc))
  if (fs.existsSync(path.join(installPath, '.ssc.env'))) {
    return
  }

  let spawnArgs
  if (os.platform() === 'win32') {
    spawnArgs = [
      'powershell.exe',
      ['.\\bin\\install.ps1', '-shbuild:$false'],
      { cwd: installPath, stdio: [process.stdin, process.stdout, process.stderr] }
    ]
  } else {
    spawnArgs = [
      './bin/functions.sh',
      ['--fte'],
      { cwd: installPath, stdio: [process.stdin, process.stdout, process.stderr] }
    ]
  }

  const child = spawn(...spawnArgs)

  await new Promise((resolve, reject) => {
    child.on('close', resolve).on('error', reject)
  })
}

export default {
  platform,
  arch,
  env,
  bin,
  firstTimeExperienceSetup
}
