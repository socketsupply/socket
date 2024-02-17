import { InvertedPromise } from '../util.js'
import state from './state.js'
import ipc from '../ipc.js'

export class ExtendableEvent extends Event {
  #promise = new InvertedPromise()
  #promises = []
  #pendingPromiseCount = 0

  waitUntil (promise) {
    // we ignore the isTrusted check here and just verify the event phase
    if (this.eventPhase !== Event.AT_TARGET) {
      throw new DOMException('Event is not active', 'InvalidStateError')
    }

    if (promise && promise instanceof Promise) {
      this.#pendingPromiseCount++
      this.#promises.push(promise)
      promise.then(
        () => queueMicrotask(() => {
          if (--this.#pendingPromiseCount === 0) {
            this.#promise.resolve()
          }
        }),
        () => queueMicrotask(() => {
          if (--this.#pendingPromiseCount === 0) {
            this.#promise.resolve()
          }
        })
      )

      // handle 0 pending promises
    }
  }

  async waitsFor () {
    if (this.#pendingPromiseCount === 0) {
      this.#promise.resolve()
    }

    return await this.#promise
  }

  get awaiting () {
    return this.waitsFor()
  }

  get pendingPromises () {
    return this.#pendingPromiseCount
  }

  get isActive () {
    return (
      this.#pendingPromiseCount > 0 ||
      this.eventPhase === Event.AT_TARGET
    )
  }
}

export class FetchEvent extends ExtendableEvent {
  #handled = new InvertedPromise()
  #request = null
  #clientId = null
  #isReload = false
  #fetchId = null

  constructor (type = 'fetch', options = null) {
    super(type, options)

    this.#fetchId = options?.fetchId ?? null
    this.#request = options?.request ?? null
    this.#clientId = options?.clientId ?? ''
    this.#isReload = options?.isReload === true
  }

  get handled () {
    return this.#handled.then(Promise.resolve())
  }

  get request () {
    return this.#request
  }

  get clientId () {
    return this.#clientId
  }

  get isReload () {
    return this.#isReload
  }

  get preloadResponse () {
    return Promise.resolve(null)
  }

  respondWith (response) {
    const clientId = this.#clientId
    const handled = this.#handled
    const id = this.#fetchId

    queueMicrotask(async () => {
      try {
        response = await response

        const arrayBuffer = await response.arrayBuffer()
        const statusCode = response.status ?? 200
        const headers = Array.from(response.headers.entries())
          .map((entry) => entry.join(':'))
          .join('\n')

        const options = { statusCode, clientId, headers, id }
        const result = await ipc.write(
          'serviceWorker.fetch.response',
          options,
          new Uint8Array(arrayBuffer)
        )

        if (result.err) {
          state.reportError(result.err)
        }

        handled.resolve()
      } catch (err) {
        state.reportError(err)
      } finally {
        handled.resolve()
      }
    })
  }
}

export default {
  ExtendableEvent,
  FetchEvent
}
