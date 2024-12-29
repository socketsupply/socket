import { MIMEParams } from './params.js'

export class MIMEType {
  #type = null
  #params = null
  #subtype = null

  constructor (input) {
    input = String(input)

    const args = input.split(';')
    const mime = args.shift()
    const types = mime.split('/')

    if (types.length !== 2 || !types[0] || !types[1]) {
      throw new TypeError(`Invalid MIMEType input given: ${mime}`)
    }

    const [type, subtype] = types

    this.#type = type.toLowerCase()
    // @ts-ignore
    this.#params = new MIMEParams(args.map((a) => a.trim().split('=').map((v) => v.trim())))
    this.#subtype = subtype
  }

  get type () {
    return this.#type
  }

  set type (value) {
    if (!value || typeof value !== 'string') {
      throw new TypeError('MIMEType type must be a string')
    }

    this.#type = value
  }

  get subtype () {
    return this.#subtype
  }

  set subtype (value) {
    if (!value || typeof value !== 'string') {
      throw new TypeError('MIMEType subtype must be a string')
    }

    this.#subtype = value
  }

  get essence () {
    return `${this.type}/${this.subtype}`
  }

  get params () {
    return this.#params
  }

  toString () {
    const params = this.params.toString()

    if (params) {
      return `${this.essence};${params}`
    }

    return this.essence
  }

  toJSON () {
    return this.toString()
  }
}

export default MIMEType
