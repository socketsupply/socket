import * as vm from '../vm.js'
import gc from '../gc.js'

let realm = null

function createTransferredError (error) {
  const object = {
    name: error.name,
    type: Object.getPrototypeOf(error).constructor.name,
    message: error.message
  }

  if (error.cause instanceof Error) {
    object.cause = createTransferredError(object.cause)
  }

  if (error.stack) {
    object.stack = error.stack
  }

  if (error.code) {
    object.code = error.code
  }

  // assign any other enumerable properties
  Object.assign(object, error)

  return object
}

const context = {}
Object.defineProperty(globalThis, 'globalObject', {
  configurable: false,
  enumerable: false,
  value: context
})

globalThis.addEventListener('message', async (event) => {
  if (!realm) {
    realm = event.source
  }

  if (event.data?.type === 'script') {
    const { id, mode, nonce, source } = event.data
    const inputTransfers = []
    const transfer = []
    let result

    vm.findMessageTransfers(inputTransfers, event.data.context, {
      ignoreScriptReferenceArgs: true
    })

    for (const value of inputTransfers) {
      if (value instanceof MessagePort) {
        return realm.postMessage({
          type: 'world.result',
          err: createTransferredError(new TypeError('MessagePort cannot be in context')),
          nonce,
          id
        })
      }
    }

    const delta = vm.applyContextDifferences(
      context,
      event.data.context,
      context,
      true
    )

    for (const key in context) {
      Object.defineProperty(globalThis, key, {
        configurable: true,
        enumerable: false,
        get: () => Reflect.get(context, key)
      })
    }

    for (const key of delta.deletions) {
      Reflect.deleteProperty(globalThis, key)
    }

    try {
      vm.applyInputContextReferences(context)
      result = await vm.compileFunction(source, {
        async: true,
        type: mode !== 'module' ? 'classic' : 'module',
        wrap: mode !== 'module',
        context
      })()
    } catch (err) {
      vm.applyOutputContextReferences(context)
      vm.findMessageTransfers(transfer, context)
      return realm.postMessage({
        type: 'world.result',
        err: createTransferredError(err),
        context,
        nonce,
        id
      }, { transfer })
    }

    if (typeof result === 'function') {
      result = vm.createReference(result, context).toJSON()
    } else if (result && typeof result === 'object') {
      if (
        Object.getPrototypeOf(result) === Object.prototype ||
        result instanceof Array
      ) {
        vm.applyOutputContextReferences(result)
      } else {
        vm.applyOutputContextReferences(result)
        result = vm.createReference(result, context).toJSON(true)
      }
    }

    vm.applyOutputContextReferences(context)
    vm.findMessageTransfers(transfer, context)
    vm.findMessageTransfers(transfer, result)

    return realm.postMessage({
      type: 'world.result',
      data: result,
      context,
      nonce,
      id
    }, { transfer })
  }

  if (event.data?.type === 'destroy') {
    const { id } = event.data
    await gc.release()
    return realm.postMessage({ type: 'world.destroy', id })
  }
})
