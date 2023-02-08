import { Path } from './path.js'

export default class POSIXPath extends Path {
  static get sep () { return '/' }
  static get delimiter () { return ':' }
}
