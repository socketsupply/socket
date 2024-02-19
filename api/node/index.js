import sugar from '../stream-relay/sugar.js'
import { Cache, Packet, sha256, Encryption, NAT } from '../stream-relay/index.js'
import events from 'events'
import dgram from 'dgram'

const network = sugar(dgram, events)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
