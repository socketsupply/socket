import channels from './channels.js'
import window from './window.js'

import * as exports from './index.js'

export default exports
export { channels, window }

/**
 * @param {string} name
 * @return {import('./channels.js').Channel}
 */
export function channel (name) {
  return channels.channel(name)
}
