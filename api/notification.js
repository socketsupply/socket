/* global CustomEvent, Event, ErrorEvent, EventTarget */
/**
 * @module Notification
 * The Notification modules provides an API to configure and display
 * desktop and mobile notifications to the user. It also includes mechanisms
 * for request permissions to use notifications on the user's device.
 */
import { Enumeration } from './enumeration.js'
import permissions from './internal/permissions.js'
import { rand64 } from './crypto.js'
import language from './language.js'
import location from './location.js'
import hooks from './hooks.js'
import URL from './url.js'
import ipc from './ipc.js'
import os from './os.js'

const isLinux = os.platform() === 'linux'
const NativeNotification = (
  globalThis.Notification ||
  class NativeNotification extends EventTarget {}
)

/**
 * Used to determine if notification beign created in a `ServiceWorker`.
 * @ignore
 */
const isServiceWorkerGlobalScope = typeof globalThis.registration?.active === 'string'

/**
 * Default number of max actions a notification can perform.
 * @ignore
 * @type {number}
 */
const DEFAULT_MAX_ACTIONS = 2

/**
 * The global event dispatched when a `Notification` is presented to
 * the user.
 * @ignore
 * @type {string}
 */
export const NOTIFICATION_PRESENTED_EVENT = 'notificationpresented'

/**
 * The global event dispatched when a `Notification` has a response
 * from the user.
 * @ignore
 * @type {string}
 */
export const NOTIFICATION_RESPONSE_EVENT = 'notificationresponse'

/**
 * A container to proxy native notification events to a runtime notication.
 * @ignore
 */
class NativeNotificationProxy extends NativeNotification {
  /**
   * @type {Notification}
   * @ignore
   */
  #notification = null

  /**
   * `NativeNotificationProxy` class constructor.
   * @param {Notification} notification
   * @ignore
   */
  constructor (notification) {
    super(notification.title, notification)

    let clicked = false
    let error = false

    this.#notification = notification

    // @ts-ignore
    this.onerror = (event) => {
      error = true
      notification.dispatchEvent(new ErrorEvent('error', {
        error: (
          event.error ||
          event.message ||
          new Error('An unknown error occured', { cause: event })
        )
      }))
    }

    // @ts-ignore
    this.onclose = (event) => {
      if (error) return
      if (!clicked) {
        const event = new CustomEvent(NOTIFICATION_RESPONSE_EVENT, {
          detail: {
            id: notification.id,
            action: 'dismiss'
          }
        })

        globalThis.dispatchEvent(event)
      }
    }

    // @ts-ignore
    this.onclick = () => {
      if (error) return
      clicked = true
      const event = new CustomEvent(NOTIFICATION_RESPONSE_EVENT, {
        detail: {
          id: notification.id,
          action: 'default'
        }
      })

      globalThis.dispatchEvent(event)
    }

    // @ts-ignore
    this.onshow = () => {
      if (error) return
      const event = new CustomEvent(NOTIFICATION_PRESENTED_EVENT, {
        detail: { id: notification.id }
      })

      globalThis.dispatchEvent(event)
    }
  }

  /**
   * The underlying `Notification` this proxy dispatches events to.
   * @type {Notification}
   * @ignore
   */
  get notification () {
    return this.#notification
  }
}

/**
 * An enumeratino of notification test directions:
 * - 'auto'  Automatically determined by the operating system
 * - 'ltr'   Left-to-right text direction
 * - 'rtl'   Right-to-left text direction
 * @type {Enumeration}
 * @ignore
 */
export const NotificationDirection = Enumeration.from([
  'auto',
  'ltr',
  'rtl'
])

/**
 * An enumeration of permission types granted by the user for the current
 * origin to display notifications to the end user.
 * - 'granted'  The user has explicitly granted permission for the current
 *              origin to display system notifications.
 * - 'denied'   The user has explicitly denied permission for the current
 *              origin to display system notifications.
 * - 'default'  The user decision is unknown; in this case the application
 *              will act as if permission was denied.
 * @type {Enumeration}
 * @ignore
 */
export const NotificationPermission = Enumeration.from([
  'granted',
  'denied',
  'default'
])

/**
 * A validated notification action object container.
 * You should never need to construct this.
 * @ignore
 */
export class NotificationAction {
  #action = null
  #title = null
  #icon = null

  /**
   * `NotificationAction` class constructor.
   * @ignore
   * @param {object} options
   * @param {string} options.action
   * @param {string} options.title
   * @param {string|URL=} [options.icon = '']
   */
  constructor (options) {
    if (options?.action === undefined) {
      throw new TypeError(
        'Failed to read the \'action\' property from ' +
        `'NotificationAction': Required member is ${options.action}.`
      )
    }

    if (options?.title === undefined) {
      throw new TypeError(
        'Failed to read the \'title\' property from ' +
        `'NotificationAction': Required member is ${options.title}.`
      )
    }

    this.#action = String(options.action)
    this.#title = String(options.title)
    this.#icon = String(options.icon ?? '')
  }

  /**
   * A string identifying a user action to be displayed on the notification.
   * @type {string}
   */
  get action () { return this.#action }

  /**
   * A string containing action text to be shown to the user.
   * @type {string}
   */
  get title () { return this.#title }

  /**
   * A string containing the URL of an icon to display with the action.
   * @type {string}
   */
  get icon () { return this.#icon }
}

/**
 * A validated notification options object container.
 * You should never need to construct this.
 * @ignore
 */
export class NotificationOptions {
  #actions = []
  #badge = ''
  #body = ''
  #data = null
  #dir = 'auto'
  #icon = ''
  #image = ''
  #lang = ''
  #renotify = false
  #requireInteraction = false
  #silent = false
  #tag = ''
  #vibrate = []

  /**
   * `NotificationOptions` class constructor.
   * @ignore
   * @param {object} [options = {}]
   * @param {string=} [options.dir = 'auto']
   * @param {NotificationAction[]=} [options.actions = []]
   * @param {string|URL=} [options.badge = '']
   * @param {string=} [options.body = '']
   * @param {?any=} [options.data = null]
   * @param {string|URL=} [options.icon = '']
   * @param {string|URL=} [options.image = '']
   * @param {string=} [options.lang = '']
   * @param {string=} [options.tag = '']
   * @param {boolean=} [options.boolean = '']
   * @param {boolean=} [options.requireInteraction = false]
   * @param {boolean=} [options.silent = false]
   * @param {number[]=} [options.vibrate = []]
   */
  constructor (options = {}) {
    if ('dir' in options) {
      // @ts-ignore
      if (!(options.dir in NotificationDirection)) {
        throw new TypeError(
          'Failed to read the \'dir\' property from \'NotificationOptions\': ' +
          `The provided value '${options.dir}' is not a valid enum value of ` +
          'type NotificationDirection.'
        )
      }

      // @ts-ignore
      this.#dir = options.dir
    }

    if ('actions' in options) {
      // @ts-ignore
      if (!(Symbol.iterator in options.actions)) {
        throw new TypeError(
          'Failed to read the \'actions\' property from ' +
          '\'NotificationOptions\': The object must have a callable ' +
          '@@iterator property.'
        )
      }

      // @ts-ignore
      for (const action of options.actions) {
        try {
          this.#actions.push(new NotificationAction(action))
        } catch (err) {
          throw new TypeError(
            'Failed to read the \'actions\' property from ' +
            `'NotificationOptions': ${err.message}`
          )
        }

        if (this.#actions.length === state.maxActions) {
          break
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
        this.#lang = language.describe(options.lang)[0]?.tag || ''
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
        this.#vibrate = options.vibrate
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

  /**
   * An array of actions to display in the notification.
   * @type {NotificationAction[]}
   */
  get actions () { return this.#actions }

  /**
   * A string containing the URL of the image used to represent
   * the notification when there isn't enough space to display the
   * notification itself.
   * @type {string}
   */
  get badge () { return this.#badge }

  /**
   * A string representing the body text of the notification,
   * which is displayed below the title.
   * @type {string}
   */
  get body () { return this.#body }

  /**
   * Arbitrary data that you want associated with the notification.
   * This can be of any data type.
   * @type {?any}
   */
  get data () { return this.#data }

  /**
   * The direction in which to display the notification.
   * It defaults to 'auto', which just adopts the environments
   * language setting behavior, but you can override that behavior
   * by setting values of 'ltr' and 'rtl'.
   * @type {'auto'|'ltr'|'rtl'}
   */
  get dir () { return this.#dir }

  /**
   * A string containing the URL of an icon to be displayed in the notification.
   * @type {string}
   */
  get icon () { return this.#icon }

  /**
   * The URL of an image to be displayed as part of the notification, as
   * specified in the constructor's options parameter.
   * @type {string}
   */
  get image () { return this.#image }

  /**
   * The notification's language, as specified using a string representing a
   * language tag according to RFC 5646.
   * @type {string}
   */
  get lang () { return this.#lang }

  /**
   * A boolean value specifying whether the user should be notified after a
   * new notification replaces an old one. The default is `false`, which means
   * they won't be notified. If `true`, then tag also must be set.
   * @type {boolean}
   */
  get renotify () { return this.#renotify }

  /**
   * Indicates that a notification should remain active until the user clicks
   * or dismisses it, rather than closing automatically.
   * The default value is `false`.
   * @type {boolean}
   */
  get requireInteraction () { return this.#requireInteraction }

  /**
   * A boolean value specifying whether the notification is silent (no sounds
   * or vibrations issued), regardless of the device settings.
   * The default is `false`, which means it won't be silent. If `true`, then
   * vibrate must not be present.
   * @type {boolean}
   */
  get silent () { return this.#silent }

  /**
   * A string representing an identifying tag for the notification.
   * The default is the empty string.
   * @type {string}
   */
  get tag () { return this.#tag }

  /**
   * A vibration pattern for the device's vibration hardware to emit with
   * the notification. If specified, silent must not be `true`.
   * @type {number[]}
   * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Vibration_API#vibration_patterns}
   */
  get vibrate () { return this.#vibrate }
}

/**
 * Show a notification. Creates a `Notification` instance and displays
 * it to the user.
 * @param {string} title
 * @param {NotificationOptions=} [options]
 * @param {function(Event)=} [onclick]
 * @param {function(Event)=} [onclose]
 * @return {Promise}
 */
export async function showNotification (title, options, onclick = null, onshow = null) {
  const notification = new Notification(title, options)

  await new Promise((resolve, reject) => {
    notification.onclick = onclick
    notification.onshow = onshow
    notification.onerror = (e) => reject(e.error)
    // @ts-ignore
    notification.onshow = () => resolve()
  })
}

/**
 * Internal state
 * @ignore
 */
const state = {
  permission: 'default',
  maxActions: DEFAULT_MAX_ACTIONS,
  pending: new Map()
}

/**
 * The Notification interface is used to configure and display
 * desktop and mobile notifications to the user.
 */
export class Notification extends EventTarget {
  /**
   * A read-only property that indicates the current permission granted
   * by the user to display notifications.
   * @type {'prompt'|'granted'|'denied'}
   */
  static get permission () {
    return state.permission
  }

  /**
   * The maximum number of actions supported by the device.
   * @type {number}
   */
  static get maxActions () {
    return state.maxActions
  }

  /**
   * Requests permission from the user to display notifications.
   * @param {object=} [options]
   * @param {boolean=} [options.alert = true] - (macOS/iOS only)
   * @param {boolean=} [options.sound = false] - (macOS/iOS only)
   * @param {boolean=} [options.badge = false] - (macOS/iOS only)
   * @param {boolean=} [options.force = false]
   * @return {Promise<'granted'|'default'|'denied'>}
   */
  static async requestPermission (options = null) {
    if (isLinux) {
      // @ts-ignore
      if (typeof NativeNotification?.requestPermission === 'function') {
        // @ts-ignore
        return await NativeNotification.requestPermission()
      }

      return 'denied'
    }

    // explicitly unsubscribe with `AbortController` to prevent
    // any result state changes further updates
    const controller = new AbortController()
    // query for 'granted' status and return early
    const query = await permissions.query({
      signal: controller.signal,
      name: 'notifications'
    })

    // if already granted, return early
    // any non-standard macOS/iOS options given will be ignored as they
    // must be configured by the user unless the request is "forced"
    if (options?.force !== true && query.state === 'granted') {
      controller.abort()
      return query.state
    }

    // request permission and resolve the normalized `state.permission` value
    // when the query status changes
    const request = await permissions.request({
      signal: controller.signal,
      name: 'notifications',

      // macOS/iOS only options
      alert: Boolean(options?.alert !== false), // (defaults to `true`)
      badge: Boolean(options?.badge),
      sound: Boolean(options?.sound)
    })

    if (request.state === 'granted') {
      controller.abort()
      return request.state
    }

    return new Promise((resolve) => {
      request.onchange = () => {
        controller.abort()
        resolve(state.permission)
      }

      query.onchange = () => {
        controller.abort()
        resolve(state.permission)
      }
    })
  }

  #onclick = null
  #onclose = null
  #onerror = null
  #onshow = null

  #options = null
  #timestamp = Date.now()
  #title = null
  #id = null

  #proxy = null

  /**
   * `Notification` class constructor.
   * @param {string} title
   * @param {NotificationOptions=} [options]
   */
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

    // @ts-ignore
    this.#id = (rand64() & 0xFFFFn).toString()

    if (isLinux) {
      const proxy = new NativeNotificationProxy(this)
      const request = new Promise((resolve) => {
        // @ts-ignore
        proxy.addEventListener('show', () => resolve({}))
        // @ts-ignore
        proxy.addEventListener('error', (e) => resolve({ err: e.error }))
      })

      this.#proxy = proxy
      this[Symbol.for('Notification.request')] = request
    } else {
      const request = ipc.request('notification.show', {
        body: this.body,
        icon: this.icon,
        id: this.#id,
        image: this.image,
        lang: this.lang,
        tag: this.tag || '',
        title: this.title,
        silent: this.silent
      })

      this[Symbol.for('Notification.request')] = request
    }

    state.pending.set(this.id, this)

    const removeNotificationPresentedListener = hooks.onNotificationPresented((event) => {
      if (event.detail.id === this.id) {
        removeNotificationPresentedListener()
        return this.dispatchEvent(new Event('show'))
      }
    })

    const removeNotificationResponseListener = hooks.onNotificationResponse((event) => {
      if (event.detail.id === this.id) {
        const eventName = event.detail.action === 'dismiss' ? 'close' : 'click'
        removeNotificationResponseListener()
        this.dispatchEvent(new Event(eventName))
        if (eventName === 'click') {
          queueMicrotask(() => this.dispatchEvent(new Event('close')))
        }
      }
    })

    // propagate error to caller
    this[Symbol.for('Notification.request')].then((result) => {
      if (result?.err) {
        // @ts-ignore
        state.pending.delete(this.id, this)
        removeNotificationPresentedListener()
        removeNotificationResponseListener()
        return this.dispatchEvent(new ErrorEvent('error', {
          message: result.err.message,
          error: result.err
        }))
      }
    })
  }

  /**
   * A unique identifier for this notification.
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * The click event is dispatched when the user clicks on
   * displayed notification.
   * @type {?function}
   */
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

  /**
   * The close event is dispatched when the notification closes.
   * @type {?function}
   */
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

  /**
   * The eror event is dispatched when the notification fails to display
   * or encounters an error.
   * @type {?function}
   */
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

  /**
   * The click event is dispatched when the notification is displayed.
   * @type {?function}
   */
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

  /**
   * An array of actions to display in the notification.
   * @type {NotificationAction[]}
   */
  get actions () { return this.#options.actions }

  /**
   * A string containing the URL of the image used to represent
   * the notification when there isn't enough space to display the
   * notification itself.
   * @type {string}
   */
  get badge () { return this.#options.badge }

  /**
   * A string representing the body text of the notification,
   * which is displayed below the title.
   * @type {string}
   */
  get body () { return this.#options.body }

  /**
   * Arbitrary data that you want associated with the notification.
   * This can be of any data type.
   * @type {?any}
   */
  get data () { return this.#options.data }

  /**
   * The direction in which to display the notification.
   * It defaults to 'auto', which just adopts the environments
   * language setting behavior, but you can override that behavior
   * by setting values of 'ltr' and 'rtl'.
   * @type {'auto'|'ltr'|'rtl'}
   */
  get dir () { return this.#options.dir }

  /**
   * A string containing the URL of an icon to be displayed in the notification.
   * @type {string}
   */
  get icon () { return this.#options.icon }

  /**
   * The URL of an image to be displayed as part of the notification, as
   * specified in the constructor's options parameter.
   * @type {string}
   */
  get image () { return this.#options.image }

  /**
   * The notification's language, as specified using a string representing a
   * language tag according to RFC 5646.
   * @type {string}
   */
  get lang () { return this.#options.lang }

  /**
   * A boolean value specifying whether the user should be notified after a
   * new notification replaces an old one. The default is `false`, which means
   * they won't be notified. If `true`, then tag also must be set.
   * @type {boolean}
   */
  get renotify () { return this.#options.renotify }

  /**
   * Indicates that a notification should remain active until the user clicks
   * or dismisses it, rather than closing automatically.
   * The default value is `false`.
   * @type {boolean}
   */
  get requireInteraction () { return this.#options.requireInteraction }

  /**
   * A boolean value specifying whether the notification is silent (no sounds
   * or vibrations issued), regardless of the device settings.
   * The default is `false`, which means it won't be silent. If `true`, then
   * vibrate must not be present.
   * @type {boolean}
   */
  get silent () { return this.#options.silent }

  /**
   * A string representing an identifying tag for the notification.
   * The default is the empty string.
   * @type {string}
   */
  get tag () { return this.#options.tag }

  /**
   * A vibration pattern for the device's vibration hardware to emit with
   * the notification. If specified, silent must not be `true`.
   * @type {number[]}
   * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Vibration_API#vibration_patterns}
   */
  get vibrate () { return this.#options.vibrate }

  /**
   * The timestamp of the notification.
   * @type {number}
   */
  get timestamp () { return this.#timestamp }

  /**
   * The title read-only property of the `Notification` instace indicates
   * the title of the notification, as specified in the `title` parameter
   * of the `Notification` constructor.
   * @type {string}
   */
  get title () { return this.#title }

  /**
   * Closes the notification programmatically.
   */
  async close () {
    if (isLinux) {
      if (this.#proxy) {
        return this.#proxy.close()
      }

      return
    }

    const result = await ipc.request('notification.close', { id: this.id })
    if (result.err) {
      console.warn('Failed to close \'Notification\': %s', result.err.message)
    }
  }
}

hooks.onReady(() => {
  if (os.host() === 'iphonesimulator' || os.host() === 'android-emulator') {
    return
  }

  // listen for 'notification' permission changes where applicable
  permissions.query({ name: 'notifications' }).then((result) => {
    result.addEventListener('change', () => {
      // 'prompt' -> 'default'
      state.permission = result.state.replace('prompt', 'default')
    })
  })
})

export default Notification
