import fs from './fs/constants.js'
import window from './window/constants.js'
export * from './fs/constants.js'
export * from './window/constants.js'
export default {
  ...fs,
  ...window
}
