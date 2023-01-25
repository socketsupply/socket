/**
 * @module P2P
 *
 * A low-level P2P module for networking that allows you to discover peers,
 * connect to peers, and send packets reliably.
 *
 * @see {@link https://github.com/socketsupply/stream-relay}
 *
 */
import { createPeer } from '@socketsupply/stream-relay'
import dgram from './dgram.js'

export * from '@socketsupply/stream-relay'
const Peer = createPeer(dgram)
export { Peer }
