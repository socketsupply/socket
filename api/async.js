/**
 * @module Async
 *
 * Various primitives for async hooks, storage, resources, and contexts.
 */
import AsyncLocalStorage from './async/storage.js'
import AsyncResource from './async/resource.js'
import AsyncContext from './async/context.js'
import Deferred from './async/deferred.js'

import {
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook,
  AsyncHook
} from './async/hooks.js'

import * as exports from './async.js'

export {
  // async resources/storages
  AsyncLocalStorage,
  AsyncResource,
  // AsyncContext
  AsyncContext,
  // deferred
  Deferred,
  // async hooks
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  createHook,
  AsyncHook
}

export default exports
