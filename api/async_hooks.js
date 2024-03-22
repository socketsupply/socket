import {
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook
} from './async/hooks.js'

import { AsyncLocalStorage } from './async/storage.js'
import { AsyncResource } from './async/resource.js'
import * as exports from './async_hooks.js'

export {
  AsyncLocalStorage,
  AsyncResource,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook
}

export default exports
