import { Enumeration } from './enumeration.js'
import language from './language.js'
import location from './location.js'
import URL from './url.js'

const isServiceWorkerGlobalScope = typeof globalThis.registration?.active === 'string'

export const NotificationDirection = new Enumeration(['auto', 'ltr', 'rtl'])

export class NotificationAction {
  #action = null
  #title = null
  #icon = null
  constructor (options) {
    if (options?.action === undefined) {
      throw new TypeError(
        'Failed to read the \'action\' property from '
        `'NotificationAction': Required member is ${options.action}.`
      )
    }

    if (options?.title === undefined) {
      throw new TypeError(
        'Failed to read the \'title\' property from '
        `'NotificationAction': Required member is ${options.title}.`
      )
    }

    this.#action = String(options.action)
    this.#title = String(options.title)
    this.#icon = String(options.icon ?? '')
  }

  get action () { return this.#action }
  get title () { return this.#title }
  get icon () { return this.#icon }
}

export class NotificationOptions {
  #actions = []
  #badge = ''
  #body = ''
  #data = null
  #dir = 'auto'
  #icon = ''
  #lang = ''
  #renotify = false
  #requireInteraction = false
  #silent = false
  #tag = ''
  #vibrate = []

  constructor (options) {
    if ('dir' in options) {
      if (!(options.dir in NotificationDirection)) {
        throw new TypeError(
          'Failed to read the \'dir\' property from \'NotificationOptions\': ' +
          `The provided value '${options.dir}' is not a valid enum value of ` +
          'type NotificationDirection.'
        )
      }

      this.#dir = options.dir
    }

    if ('actions' in options) {
      if (!(Symbol.iterator in options.actions)) {
        throw new TypeError(
          'Failed to read the \'actions\' property from ' +
          '\'NotificationOptions\': The object must have a callable ' +
          '@@iterator property.'
        )
      }

      for (const action of options.actions) {
        try {
          this.#actions.push(new NotificationAction(action))
        } catch (err) {
          throw new TypeError(
            'Failed to read the \'actions\' property from ' +
            `'NotificationOptions': ${err.message}`
          )
        }
      }
    }

    if (this.#actions.length && !isServiceWorkerGlobalScope) {
      throw new TypeError(
        'Failed to construct \'Notification\': Actions are only supported ' +
        'for persistent notifications shown using ' +
        'ServiceWorkerRegistration.showNotification().'
      )
    }

    if ('badge' in options && options.badge !== undefined) {
      this.#badge = String(new URL(String(options.badge), location.href))
    }

    if ('body' in options && options.body !== undefined) {
      this.#body = String(options.body)
    }

    if ('data' in options && options.data !== undefined) {
      this.#data = options.data
    }

    if ('icon' in options && options.icon !== undefined) {
      this.#icon = String(new URL(String(options.icon), location.href))
    }

    if ('image' in options && options.image !== undefined) {
      this.#image = String(new URL(String(options.image), location.href))
    }

    if ('lang' in options && options.lang !== undefined) {
      if (typeof options.lang === 'string' && options.lang.length > 2) {
        this.#lang = language.describe(options.lang).[0]?.tag || ''
      }
    }

    if ('tag' in options && options.tag !== undefined) {
      this.#tag = String(options.tag)
    }

    if ('renotify' in options && options.renotify !== undefined) {
      this.#renotify = Boolean(options.renotify)

      if (this.#renotify === true && !this.#tag.length) {
        throw new TypeError(
          'Notifications which set the renotify flag must specify a non-empty tag.'
        )
      }
    }

    if ('requireInteraction' in options && options.requireInteraction !== undefined) {
      this.#requireInteraction = Boolean(options.requireInteraction)
    }

    if ('silent' in options && options.silent !== undefined) {
      this.#silent = Boolean(options.silent)
    }

    if ('vibrate' in options && options.vibrate !== undefined) {
      if (Array.isArray(options.vibrate)) {
        this.#vibrate = this.#vibrate
      } else if (options.vibrate) {
        this.#vibrate = [options.vibrate]
      } else {
        this.#vibrate = [0]
      }

      if (this.#vibrate.length) {
        throw new TypeError(
          'Silent notifications must not specify vibration patterns.'
        )
      }

      this.#vibrate = this.#vibrate
        .map((v) => parseInt(v) || 0)
        .map((v) => Math.min(v, 10000))
        .map((v) => v < 0 ? 10000 : v)
    }
  }

  get actions () { return this.#actions }
  get badge () { return this.#badge }
  get body () { return this.#body }
  get data () { return this.#data }
  get dir () { return this.#dir }
  get icon () { return this.#icon }
  get lang () { return this.#lang }
  get renotify () { return this.#renotify }
  get requireInteraction () { return this.#requireInteraction }
  get silent () { return this.#silent }
  get tag () { return this.#tag }
  get vibrate () { return this.#vibrate }
}

export class Notification extends EventTarget {
  static get permission () {
  }

  #onclick = null
  #onclose = null
  #onerror = null
  #onshow = null

  #options = null
  #timestamp = Date.now()
  #title = null

  constructor (title, options = {}) {
    super()
    if (arguments.length === 0) {
      throw new TypeError(
        'Failed to construct \'Notification\': ' +
        '1 argument required, but only 0 present.'
      )
    }

    if (options === null || options === undefined) {
      options = {}
    }

    this.#title = String(title)

    if (typeof options !== 'object') {
      throw new TypeError(
        'Failed to construct \'Notification\': ' +
        'The provided value is not of type \'NotificationOptions\'.'
      )
    }

    try {
      this.#options = new NotificationOptions(options)
    } catch (err) {
      throw new TypeError(
        `Failed to construct 'Notification': ${err.message}`
      )
    }
  }

  get onclick () { return this.#onclick }
  set onclick (onclick) {
    if (this.#onclick === onclick) {
      return
    }

    if (this.#onclick) {
      this.removeEventListener('click', this.#onclick)
      this.#onclick = null
    }

    if (typeof onclick === 'function') {
      this.#onclick = onclick
      this.addEventListener('click', onclick)
    }
  }

  get onclose () { return this.#onclose }
  set onclose (onclose) {
    if (this.#onclose === onclose) {
      return
    }

    if (this.#onclose) {
      this.removeEventListener('close', this.#onclose)
      this.#onclose = null
    }

    if (typeof onclose === 'function') {
      this.#onclose = onclose
      this.addEventListener('close', onclose)
    }
  }

  get onerror () { return this.#onerror }
  set onerror (onerror) {
    if (this.#onerror === onerror) {
      return
    }

    if (this.#onerror) {
      this.removeEventListener('error', this.#onerror)
      this.#onerror = null
    }

    if (typeof onerror === 'function') {
      this.#onerror = onerror
      this.addEventListener('error', onerror)
    }
  }

  get onshow () { return this.#onshow }
  set onshow (onshow) {
    if (this.#onshow === onshow) {
      return
    }

    if (this.#onshow) {
      this.removeEventListener('show', this.#onshow)
      this.#onshow = null
    }

    if (typeof onshow === 'function') {
      this.#onshow = onshow
      this.addEventListener('show', onshow)
    }
  }

  get actions () { return this.#options.actions }
  get badge () { return this.#options.badge }
  get body () { return this.#options.body }
  get data () { return this.#options.data }
  get dir () { return this.#options.dir }
  get icon () { return this.#options.icon }
  get lang () { return this.#options.lang }
  get renotify () { return this.#options.renotify }
  get requireInteraction () { return this.#options.requireInteraction }
  get silent () { return this.#silent }
  get tag () { return this.#options.tag }
  get timestamp () { return this.#timestamp }
  get title () { return this.#title }
  get vibrate () { return this.#options.vibrate }
}

export default Notification
