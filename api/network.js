/**
 * @module Network
 *
 * Provides a higher level API over the stream-relay protocol.
 *
 * @see {@link https://socketsupply.co/guides/#p2p-guide}
 *
 */
import sugar from './stream-relay/sugar.js'
import { Cache, Packet, sha256, Encryption, NAT } from './stream-relay/index.js'
import events from './events.js'
import dgram from './dgram.js'

const network = sugar(dgram, events)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
