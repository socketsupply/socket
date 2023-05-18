import { ErrorEvent } from 'socket:events'
import hooks from 'socket:hooks'
import test from 'socket:test'

const initial = {
  isDocumentReady: hooks.isDocumentReady,
  isRuntimeReady: hooks.isRuntimeReady,
  isGlobalReady: hooks.isGlobalReady,
  isReady: hooks.isReady
}

const callbacks = {
  onReady () { callbacks.onReady.called = true },
  onLoad () { callbacks.onLoad.called = true },
  onInit () { callbacks.onInit.called = true }
}

class TestIgnoredError extends Error {
  [Symbol.for('socket.test.error.ignore')] = true
}

hooks.onReady(callbacks.onReady)
hooks.onLoad(callbacks.onLoad)
hooks.onInit(callbacks.onInit)

test('hooks - initial state', async (t) => {
  t.ok(initial.isDocumentReady === false, 'isDocumentReady === false')
  t.ok(initial.isRuntimeReady === false, 'isRuntimeReady === false')
  t.ok(initial.isGlobalReady === false, 'isGlobalReady === false')
  t.ok(initial.isReady === false, 'isReady === false')
})

test('hooks - properties after load', async (t) => {
  t.ok(hooks.global === globalThis, 'hooks.global')
  t.ok(hooks.document === globalThis.document, 'hooks.document')
  t.ok(hooks.window === globalThis.window, 'hooks.window')
  t.ok(hooks.isDocumentReady === true, 'hooks.isDocumentReady === true')
  t.ok(hooks.isGlobalReady === true, 'hooks.isGlobalReady === true')
  t.ok(hooks.isRuntimeReady === true, 'hooks.isRuntimeReady === true')
  t.ok(hooks.isReady === true, 'hooks.isReady === true')
  t.ok(typeof hooks.isOnline === 'boolean', 'typeof hooks.isOnline === boolean')
  t.ok(typeof hooks.isWorkerContext === 'boolean', 'typeof hooks.isWorkerContext === boolean')
  t.ok(typeof hooks.isWindowContext === 'boolean', 'typeof hooks.isWindowContext === boolean')
})

test('hooks - callbacks called during load', async (t) => {
  t.ok(callbacks.onReady.called === true, 'onReady called')
  t.ok(callbacks.onLoad.called === true, 'onLoad called')
  t.ok(callbacks.onInit.called === true, 'onInit called')
})

test('hooks - callbacks called after load', async (t) => {
  const pending = []
  pending.push(new Promise((resolve) => hooks.onInit(resolve)))
  pending.push(new Promise((resolve) => hooks.onLoad(resolve)))
  pending.push(new Promise((resolve) => hooks.onReady(resolve)))
  await Promise.all(pending)
  t.pass('onInit, onLoad, onReady called')
})

test('hooks - error callback for global errors', async (t) => {
  let pending = 3
  queueMicrotask(() => { throw new TestIgnoredError('oops') })
  setTimeout(() => Promise.reject(new TestIgnoredError('oopsies')))
  Promise.resolve().then(() => {
    globalThis.dispatchEvent(new ErrorEvent('messageerror', {
      error: new TestIgnoredError('ouch')
    }))
  })
  await new Promise((resolve) => {
    hooks.onError((event) => {
      if (--pending === 0) {
        resolve()
      }
    })
  })

  t.pass('error, messageerror, and unhandledrejection handled')
})
