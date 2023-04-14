/**
 * @module Bluetooth
 *
 * A high-level, cross-platform API for Bluetooth Pub-Sub
 *
 * Example usage:
 * ```js
 * import { Bluetooth } from 'socket:bluetooth'
 * ```
 */

import { EventEmitter } from './events.js'
import diagnostics from './diagnostics.js'
import ipc from './ipc.js'

import * as exports from './bluetooth.js'

export default exports

const dc = diagnostics.channels.group('bluetooth', [
  'data',
  'event',
  'start',
  'handle',
  'publish',
  'subscribe'
])

/**
 * Create an instance of a Bluetooth service.
 */
export class Bluetooth extends EventEmitter {
  static isInitalized = false

  /**
   * constructor is an example property that is set to `true`
   * Creates a new service with key-value pairs
   * @param {string} serviceId - Given a default value to determine the type
   */
  constructor (serviceId = '') {
    super()

    if (!serviceId || serviceId.length !== 36) {
      throw new Error('expected serviceId of length 36')
    }

    this.serviceId = serviceId

    window.addEventListener('data', e => {
      if (!e.detail.params) return
      const { err, data } = e.detail.params

      if (err) return this.emit('error', err)

      if (data?.serviceId === this.serviceId) {
        this.emit(data.characteristicId, data, e.detail.data)
        dc.channel('data').publish({
          bluetooth: this,
          serviceId,
          characteristicId: data.characteristicId,
          data: e.detail.data
        })
      }
    })

    window.addEventListener('bluetooth', e => {
      if (typeof e.detail !== 'object') return
      const { err, data } = e.detail

      if (err) {
        return this.emit('error', err)
      }

      this.emit(data.event, data)
      dc.channel('event').publish({
        ...data,
        bluetooth: this,
        event: data.event
      })
    })

    dc.channel('handle').publish({ bluetooth: this })
  }

  /**
   * Start the Bluetooth service.
   * @return {Promise<ipc.Result>}
   *
   */
  async start () {
    const result = await ipc.send('bluetooth.start', { serviceId: this.serviceId })

    if (result.err) {
      throw result.err
    }

    dc.channel('start').publish({
      bluetooth: this,
      serviceId: this.serviceId
    })
  }

  /**
   * Start scanning for published values that correspond to a well-known UUID.
   * Once subscribed to a UUID, events that correspond to that UUID will be
   * emitted. To receive these events you can add an event listener, for example...
   *
   * ```js
   * const ble = new Bluetooth(id)
   * ble.subscribe(uuid)
   * ble.on(uuid, (data, details) => {
   *   // ...do something interesting
   * })
   * ```
   *
   * @param {string} [id = ''] - A well-known UUID
   * @return {Promise<ipc.Result>}
   */
  async subscribe (id = '') {
    const result = await ipc.send('bluetooth.subscribe', {
      characteristicId: id,
      serviceId: this.serviceId
    })

    if (result.err) {
      throw result.err
    }

    dc.channel('subscribe').publish({
      bluetooth: this,
      serviceId: this.serviceId,
      characteristicId: id
    })
  }

  /**
   * Start advertising a new value for a well-known UUID
   * @param {string} [id=''] - A well-known UUID
   * @param {string} [value='']
   * @return {Promise<void>}
   */
  async publish (id = '', value = '') {
    if (!id || id.length !== 36) {
      throw new Error('expected id of length 36')
    }

    const params = {
      characteristicId: id,
      serviceId: this.serviceId
    }

    if (!(value instanceof ArrayBuffer) && typeof value === 'object') {
      value = JSON.stringify(value)
    }

    if (typeof value === 'string') {
      const enc = new TextEncoder().encode(value)
      value = enc
      params.length = enc.length
    }

    const result = await ipc.write('bluetooth.publish', params, value)

    if (result.err) {
      throw result.err
    }

    dc.channel('publish').publish({
      bluetooth: this,
      serviceId: this.serviceId,
      characteristicId: id,
      data: value
    })
  }
}
