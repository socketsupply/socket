import channels from './channels.js'
import window from './window.js'
import runtime from './runtime.js'

import * as exports from './index.js'

export default exports
export { channels, window, runtime }

/**
 * @param {string} name
 * @return {import('./channels.js').Channel}
 */
export function channel (name) {
  return channels.channel(name)
}
