import AsyncContext from '../async-context.js'
import { wrap as asyncWrap } from './async.js'

let currentAsyncResourceId = 1

export const state = {
  defaultExecutionAsyncId: -1
}

export const hooks = {
  init: [],
  before: [],
  after: [],
  destroy: [],
  promiseResolve: []
}

export class TopLevelAsyncResource {}
export const topLevelAsyncResource = new TopLevelAsyncResource()

export const asyncResources = new Map()
export const asyncContextSnapshot = new AsyncContext.Snapshot()
export const asyncContextVariable = new AsyncContext.Variable({
  name: 'async-hooks',
  defaultValue: topLevelAsyncResource
})

export function dispatch (hook, asyncId, type, triggerAsyncId, resource) {
  if (hook in hooks) {
    for (const callback of hooks[hook]) {
      switch (callback.length) {
        case 4: callback(asyncId, type, triggerAsyncId, resource); break
        case 3: callback(asyncId, type, triggerAsyncId); break
        case 2: callback(asyncId, type); break
        case 1: callback(asyncId); break
      }
    }
  }
}

export function getNextAsyncResourceId () {
  return ++currentAsyncResourceId
}

export function executionAsyncResource () {
  return asyncContextVariable.get()
}

export function executionAsyncId () {
  const resource = asyncResources.get(executionAsyncResource())
  if (resource && typeof resource.asyncId === 'function') {
    return resource.asyncId()
  }

  return 1 // top level
}

export function triggerAsyncId () {
  const revert = asyncContextVariable.revert
  if (revert?.previousVariable) {
    const resource = revert.previousVariable.get()
    if (resource && typeof resource.asyncId === 'function') {
      return resource.asyncId()
    }
  }

  return 1 // top level
}

export function getDefaultExecutionAsyncId () {
  if (state.defaultExecutionAsyncId < 0) {
    return executionAsyncId()
  }

  return state.defaultExecutionAsyncId
}

export function wrap (
  callback,
  type,
  asyncId = getNextAsyncResourceId(),
  triggerAsyncId = getDefaultExecutionAsyncId(),
  resource = undefined
) {
  dispatch('init', asyncId, type, triggerAsyncId, resource)
  callback = asyncWrap(callback)
  return function (...args) {
    dispatch('before', asyncId)
    try {
      // eslint-disable-next-line
      return callback(...args)
    } finally {
      dispatch('after', asyncId)
    }
  }
}

export default hooks
