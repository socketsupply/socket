export * as backend from './backend.js'
export { Bluetooth } from './bluetooth.js'
export { bootstrap } from './bootstrap.js'
export * as buffer from './buffer.js'
export { Buffer } from './buffer.js'
export { default as console } from './console.js'
export * as crypto from './crypto.js'
export * as dgram from './dgram.js'
export * as dns from './dns.js'
export * as errors from './errors.js'
export * as events from './events.js'
export { EventEmitter } from './events.js'
export * as fs from './fs.js'
export { default as gc } from './gc.js'
export * as ipc from './ipc.js'
export * as os from './os.js'
export { default as path } from './path.js'
export { default as process } from './process.js'
export * as runtime from './runtime.js'
export * as stream from './stream.js'
export * as util from './util.js'

export const Socket = Object.create(Object.prototype, {
  constructor: {
    value: class Socket {}
  }
})

export default Socket

// eslint-disable-next-line
import * as exports from './index.js'
for (const key in exports) {
  if (key !== 'default') {
    Socket[key] = exports[key]
  }
}

Object.freeze(Socket)
