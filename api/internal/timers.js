export function setImmediate (callback, ...args) {
  return setTimeout(callback, 0, ...args)
}

export function clearImmediate (immediate) {
  return clearTimeout(immediate)
}

export default {
  setImmediate,
  clearTimeout
}
