import sugar from '../stream-relay/sugar.js'
import { Cache, Packet, sha256, Encryption, NAT } from '../stream-relay/index.js'
import events from 'node:events'
import dgram from 'node:dgram'

const network = sugar(dgram, events)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
