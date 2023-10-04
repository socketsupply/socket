/**
 * @module i18n
 * Functions to internationalize your application.
 * These module APIs can be used to get localized strings from locale files
 * packaged with your application, find out the browser's current language, and find out the value of its Accept-Language header.
 */

/* global XMLHttpRequest */
import Enumeration from './enumeration.js'
import application from './application.js'
import language from './language.js'
import location from './location.js'
import hooks from './hooks.js'

// preload environment languages when environment is ready and listen
// for language change events
hooks.onReady(preloadUILanguage)
hooks.onLanguageChange(preloadUILanguage)

/**
 * Preloads current UI language into cache.
 * @ignore
 */
function preloadUILanguage () {
  for (const language of getAcceptLanguages()) {
    queueMicrotask(() => getMessagesForLocale(language))
  }
}

/**
 * A cache of loaded locale messages.
 * @type {Map}
 */
export const cache = new Map()

/**
 * Default location of i18n locale messages
 * @type {string}
 */
export const DEFAULT_LOCALES_LOCATION = (
  application.config.i18n_locales_default ||
  'i18n/locales'
)

/**
 * Get messages for `locale` pattern. This function could return many results
 * for various locales given a `locale` pattern. such as `fr`, which could
 * return results for `fr`, `fr-FR`, `fr-BE`, etc.
 * @ignore
 * @param {string} locale
 * @return {object[]}
 */
export function getMessagesForLocale (locale) {
  const results = []
  const tags = language.lookup(locale)
    .map((l) => l.tags)
    .reduce((a, t) => a.concat(t), [])

  while (tags.length) {
    const tag = tags.shift()
    const source = (
      application.config[`i18n_locales_${tag}_source`] ||
      application.config[`i18n_locales_${tag.toLowerCase()}_source`] ||
      application.config[`i18n_locales_${tag}`] ||
      application.config[`i18n_locales_${tag.toLowerCase()}`] ||
      `${DEFAULT_LOCALES_LOCATION}/${tag}`
    )

    const path = source.endsWith('/') ? source : source + '/'
    const url = String(new URL('messages.json', new URL(path, location.origin)))

    if (cache.has(url)) {
      results.push(cache.get(url))
      continue
    }

    const request = new XMLHttpRequest()

    try {
      request.open('GET', url, false)
      request.send(null)
    } catch (err) {
      console.warn(err.message)
      continue
    }

    let result = null

    // `request.responseText` can throw `InvalidStateError` error
    try {
      result = {
        locale: tag,
        // @ts-ignore
        messages: JSON.parse(request.responseText)
      }
    } catch {
      try {
        result = {
          locale: tag,
          messages: JSON.parse(String(request.response))
        }
      } catch {
        continue
      }
    }

    if (result) {
      results.push(result)
      cache.set(url, result)
    }
  }

  return results
}

/**
 * An enumeration of supported ISO 639 language codes or RFC 5646 language tags.
 * @type {Enumeration}
 * @see {@link https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/API/i18n/LanguageCode}
 * @see {@link https://developer.chrome.com/docs/extensions/reference/i18n/#type-LanguageCode}
 */
export const LanguageCode = Enumeration.from(language.tags)

/**
 * Returns user preferred ISO 639 language codes or RFC 5646 language tags.
 * @return {string[]}
 */
export function getAcceptLanguages () {
  return globalThis.navigator?.languages ?? []
}

/**
 * Returns the current user ISO 639 language code or RFC 5646 language tag.
 * @return {?string}
 */
export function getUILanguage () {
  return globalThis.navigator?.language ?? null
}

/**
 * Gets a localized message string for the specified message name.
 * @param {string} messageName
 * @param {object|string[]=} [substitutions = []]
 * @param {object=} [options]
 * @param {string=} [options.locale = null]
 * @see {@link https://developer.chrome.com/docs/extensions/reference/i18n/#type-LanguageCode}
 * @see {@link https://www.ibm.com/docs/en/rbd/9.5.1?topic=syslib-getmessage}
 * @return {?string}
 */
export function getMessage (
  messageName,
  substitutions = [],
  options = { locale: null }
) {
  if (typeof substitutions === 'object' && !Array.isArray(substitutions)) {
    options = substitutions
  }

  const locale = options?.locale ?? getUILanguage()
  const results = getMessagesForLocale(locale)

  for (const result of results) {
    if (messageName in result.messages) {
      const { message, placeholders } = result.messages[messageName]
      let interpolated = message

      for (let i = 0; i < substitutions.length; ++i) {
        interpolated = interpolated
          .replace(new RegExp(`\\{${i}\\}`, 'g'), substitutions[i])
          .replace(new RegExp(`\\$${i}\\$?`, 'g'), substitutions[i])
      }

      if (placeholders && typeof placeholders === 'object') {
        for (const key in placeholders) {
          const placeholder = placeholders[key]
          let content = placeholder.content || placeholder

          // interpolate placeholder content
          for (let i = 0; i < substitutions.length; ++i) {
            content = content
              .replace(new RegExp(`\\{${i}\\}`, 'g'), substitutions[i])
              .replace(new RegExp(`\\$${i}\\$?`, 'g'), substitutions[i])
          }

          interpolated = interpolated
            .replace(new RegExp(`\\{${key}\\}`, 'gi'), content)
            .replace(new RegExp(`\\$${key}\\$?`, 'gi'), content)
        }
      }

      return interpolated
    }
  }

  return null
}

/**
 * Gets a localized message description string for the specified message name.
 * @param {string} messageName
 * @param {object=} [options]
 * @param {string=} [options.locale = null]
 * @return {?string}
 */
export function getMessageDescription (
  messageName,
  options = { locale: null }
) {
  const locale = options?.locale ?? getUILanguage()
  const results = getMessagesForLocale(locale)

  for (const result of results) {
    if (messageName in result.messages) {
      const { description } = result.messages[messageName]
      return description || null
    }
  }

  return null
}

export default {
  LanguageCode,
  getAcceptLanguages,
  getMessage,
  getUILanguage
}
