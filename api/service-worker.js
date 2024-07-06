import { ExtendableEvent, FetchEvent } from './service-worker/events.js'
import { Environment } from './service-worker/env.js'
import { Context } from './service-worker/context.js'

/**
 * A reference to the opened environment. This value is an instance of an
 * `Environment` if the scope is a ServiceWorker scope.
 * @type {Environment|null}
 */
export const env = Environment.instance

export {
  ExtendableEvent,
  FetchEvent,
  Environment,
  Context
}

export default {
  ExtendableEvent,
  FetchEvent,
  Environment,
  Context,
  env
}
