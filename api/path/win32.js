import { Path } from './path.js'

export default class Win32Path extends Path {
  static get sep () { return '\\' }
  static get delimiter () { return ';' }
}
