import { posix, win32 } from './path/index.js'
import os from './os.js'

export * from './path/index.js'

export default class Path extends (os.platform() === 'win32' ? win32 : posix) {
  static get win32 () { return win32 }
  static get posix () { return posix }
}
