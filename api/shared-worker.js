import { SharedWorker } from './shared-worker/index.js'
import { Environment } from './shared-worker/env.js'

/**
 * A reference to the opened environment. This value is an instance of an
 * `Environment` if the scope is a ServiceWorker scope.
 * @type {Environment|null}
 */
export const env = Environment.instance

export {
  Environment,
  SharedWorker
}

export default SharedWorker
