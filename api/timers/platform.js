export const platform = {
  setTimeout: globalThis.setTimeout.bind(globalThis),
  setInterval: globalThis.setInterval.bind(globalThis),
  setImmediate: globalThis.setTimeout.bind(globalThis),
  clearTimeout: globalThis.clearTimeout.bind(globalThis),
  clearInterval: globalThis.clearInterval.bind(globalThis),
  clearImmediate: globalThis.clearTimeout.bind(globalThis),
  postTask: (
    globalThis.scheduler?.postTask?.bind?.(globalThis?.scheduler) ??
    async function notSupported () {}
  )
}

export default platform
