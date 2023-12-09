import { primordials } from './ipc.js'
import {
  Path,
  posix,
  win32,

  // well known
  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC,
  HOME
} from './path/index.js'

const isWin32 = primordials.platform === 'win32'

export const sep = isWin32 ? win32.sep : posix.sep
export const delimiter = isWin32 ? win32.delimiter : posix.delimiter

export const resolve = isWin32 ? win32.resolve : posix.resolve
export const join = isWin32 ? win32.join : posix.join
export const dirname = isWin32 ? win32.dirname : posix.dirname
export const basename = isWin32 ? win32.basename : posix.basename
export const extname = isWin32 ? win32.extname : posix.extname
export const cwd = isWin32 ? win32.cwd : posix.cwd
export const isAbsolute = isWin32 ? win32.isAbsolute : posix.isAbsolute
export const parse = isWin32 ? win32.parse : posix.parse
export const format = isWin32 ? win32.format : posix.format
export const normalize = isWin32 ? win32.normalize : posix.normalize
export const relative = isWin32 ? win32.relative : posix.relative

export {
  Path,
  posix,
  win32,

  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC,
  HOME
}

export default isWin32 ? win32 : posix
