import { setImmediate, setTimeout } from './promises.js'

const platform = {
  postTask: (
    globalThis.scheduler?.postTask?.bind?.(globalThis?.scheduler) ??
    async function notSupported () {}
  )
}

export async function wait (delay, options = null) {
  return await setTimeout(delay, undefined, options)
}

export async function postTask (callback, options = null) {
  return await platform.postTask(callback, options)
}

export default {
  postTask,
  yield: setImmediate,
  wait
}
