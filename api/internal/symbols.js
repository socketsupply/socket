export const dispose = Symbol.dispose ?? Symbol.for('socket.runtime.gc.finalizer')
export const serialize = Symbol.serialize ?? Symbol.for('socket.runtime.serialize')

export default {
  dispose,
  serialize
}
