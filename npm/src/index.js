import path from 'node:path'
import os from 'node:os'

const dirname = path.dirname(import.meta.url.replace('file://', ''))

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

export default {
  platform,
  arch,
  env,
  bin
}
