/**
 * @module P2P
 *
 * A low-level network protocol for P2P.
 *
 * @see {@link https://github.com/socketsupply/stream-relay}
 *
 */
import { createPeer } from './stream-relay/index.js'

import dgram from './dgram.js'

const Peer = createPeer(dgram)

export * from './stream-relay/index.js'
export { Peer }
