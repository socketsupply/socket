/**
 * A container for MIME types by class (audio, video, text, etc)
 * @see {@link https://www.iana.org/assignments/media-types/media-types.xhtml}
 */
export class Database {
  /**
   * @type {string}
   */
  name = ''

  /**
   * @type {URL}
   */
  url = null

  /**
   * @type {Map}
   */
  map = null

  /**
   * `Database` class constructor.
   * @param {string} name
   */
  constructor (name) {
    // @ts-ignore
    this.url = new URL(`${name}.json`, import.meta.url)
    this.map = new Map()
    this.name = name
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
      }
    }
  }

  /**
   * Lookup MIME type by name.
   * @param {string} query
   * @return {Promise<{ name: string, mime: string}>}
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
        results.push({ name, mime })
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
            results.push({ name, mime })
          }
        }
      }
    }

    return results
  }
}

export const application = new Database('application')
export const audio = new Database('audio')
export const font = new Database('font')
export const image = new Database('image')
export const model = new Database('model')
export const multipart = new Database('multipart')
export const text = new Database('text')
export const video = new Database('video')

/**
 * Look up a MIME type in various MIME databases.
 * @return {Promise<Array<{ name: string, mime: string }>>}
 */
export async function lookup (query) {
  const databases = [
    application,
    audio,
    font,
    image,
    model,
    multipart,
    text,
    video
  ]

  const results = []
  for (const database of databases) {
    const result = await database.lookup(query)
    results.push(...result)
  }

  return results
}

export default {
  application,
  Database
}
