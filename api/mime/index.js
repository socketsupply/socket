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
   * Lookup MIME type by name or content type
   * @param {string} query
   * @return {Promise<DatabaseQueryResult>}
   */
  async lookup (query) {
    query = query.toLowerCase()

    await this.load()

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

  // preload all databasees
  await Promise.all(databases.map((db) => db.load()))

  for (const database of databases) {
    const result = await database.lookup(query)
    results.push(...result)
  }

  return results
}

export default {
  // API
  Database,
  databases,
  lookup,

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
