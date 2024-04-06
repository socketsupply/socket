/* global XMLHttpRequest */
/**
 * A container for a database lookup query.
 */
export class DatabaseQueryResult {
  /**
   * @type {string}
   */
  name = ''

  /**
   * @type {string}
   */
  mime = ''

  /**
   * `DatabaseQueryResult` class constructor.
   * @ignore
   * @param {Database} database
   * @param {string} name
   * @param {string} mime
   */
  constructor (database, name, mime) {
    this.database = database
    this.name = name
    this.mime = mime
  }
}

/**
 * A container for MIME types by class (audio, video, text, etc)
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml}
 */
export class Database {
  /**
   * The name of the MIME database.
   * @type {string}
   */
  name = ''

  /**
   * The URL of the MIME database.
   * @type {URL}
   */
  url = null

  /**
   * The mapping of MIME name to the MIME "content type"
   * @type {Map}
   */
  map = new Map()

  /**
   * An index of MIME "content type" to the MIME name.
   * @type {Map}
   */
  index = new Map()

  /**
   * `Database` class constructor.
   * @param {string} name
   */
  constructor (name) {
    // @ts-ignore
    this.url = new URL(`${name}.json`, import.meta.url)
    this.name = name
  }

  /**
   * An enumeration of all database entries.
   * @return {Array<Array<string>>}
   */
  entries () {
    return this.map.entries()
  }

  /**
   * Loads database MIME entries into internal map.
   * @return {Promise}
   */
  async load () {
    if (this.map.size === 0) {
      const response = await fetch(this.url)
      const json = await response.json()

      for (const [key, value] of Object.entries(json)) {
        this.map.set(key, value)
        this.index.set(value.toLowerCase(), key.toLowerCase())
      }
    }
  }

  /**
   * Loads database MIME entries synchronously into internal map.
   */
  loadSync () {
    if (this.map.size === 0) {
      const request = new XMLHttpRequest()

      request.open('GET', this.url, false)
      request.send(null)

      let responseText = null

      try {
        // @ts-ignore
        responseText = request.responseText // can throw `InvalidStateError` error
      } catch {
        responseText = request.response
      }

      const json = JSON.parse(responseText)

      for (const [key, value] of Object.entries(json)) {
        this.map.set(key, value)
        this.index.set(value.toLowerCase(), key.toLowerCase())
      }
    }
  }

  /**
   * Lookup MIME type by name or content type
   * @param {string} query
   * @return {Promise<DatabaseQueryResult[]>}
   */
  async lookup (query) {
    await this.load()
    return this.query(query)
  }

  /**
   * Lookup MIME type by name or content type synchronously.
   * @param {string} query
   * @return {Promise<DatabaseQueryResult[]>}
   */
  lookupSync (query) {
    this.loadSync()
    return this.query(query)
  }

  /**
   * Queries database map and returns an array of results
   * @param {string} query
   * @return {DatabaseQueryResult[]}
   */
  query (query) {
    query = query.toLowerCase()

    const queryParts = query.split('+')
    const results = []

    for (const [key, value] of this.map.entries()) {
      const name = key.toLowerCase()
      const mime = value.toLowerCase()

      if (query === name) {
        results.push(new DatabaseQueryResult(this, name, mime))
      } else if (query === mime) { // reverse lookup
        results.push(new DatabaseQueryResult(this, name, mime))
      } else if (name.startsWith(query)) {
        const nameParts = key.toLowerCase().split('+')
        if (queryParts.length <= nameParts.length) {
          let match = false

          for (let i = 0; i < queryParts.length; ++i) {
            if (queryParts[i] === nameParts[i]) {
              match = true
            } else {
              match = false
              break
            }
          }

          if (match) {
            results.push(new DatabaseQueryResult(this, name, mime))
          }
        }
      }
    }

    return results
  }
}

/**
 * A database of MIME types for 'application/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#application}
 */
export const application = new Database('application')

/**
 * A database of MIME types for 'audio/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#audio}
 */
export const audio = new Database('audio')

/**
 * A database of MIME types for 'font/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#font}
 */
export const font = new Database('font')

/**
 * A database of MIME types for 'image/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#image}
 */
export const image = new Database('image')

/**
 * A database of MIME types for 'model/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#model}
 */
export const model = new Database('model')

/**
 * A database of MIME types for 'multipart/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#multipart}
 */
export const multipart = new Database('multipart')

/**
 * A database of MIME types for 'text/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#text}
 */
export const text = new Database('text')

/**
 * A database of MIME types for 'video/' content types
 * @type {Database}
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml#video}
 */
export const video = new Database('video')

/**
 * An array of known MIME databases. Custom databases can be added to this
 * array in userspace for lookup with `mime.lookup()`
 * @type {Database[]}
 */
export const databases = [
  application,
  audio,
  font,
  image,
  model,
  multipart,
  text,
  video
]

/**
 * Look up a MIME type in various MIME databases.
 * @param {string} query
 * @return {Promise<DatabaseQueryResult[]>}
 */
export async function lookup (query) {
  const results = []

  // preload all databases
  await Promise.all(databases.map((db) => db.load()))

  for (const database of databases) {
    const result = await database.lookup(query)
    results.push(...result)
  }

  return results
}

/**
 * Look up a MIME type in various MIME databases synchronously.
 * @param {string} query
 * @return {DatabaseQueryResult[]}
 */
export async function lookupSync (query) {
  const results = []

  for (const database of databases) {
    const result = database.lookupSync(query)
    results.push(...result)
  }

  return results
}

export class MIMEParams extends Map {
  toString () {
    return Array
      .from(this.entries())
      .map((entry) => entry.join('='))
      .join(';')
  }
}

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

export default {
  // API
  Database,
  databases,
  lookup,
  lookupSync,
  MIMEParams,
  MIMEType,

  // databases
  application,
  audio,
  font,
  image,
  model,
  multipart,
  text,
  video
}
