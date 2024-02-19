import sugar from '../stream-relay/sugar.js'
import { Cache, Packet, sha256, Encryption, NAT } from '../stream-relay/index.js'
import worker from 'node:worker_threads'
import events from 'node:events'
import dgram from 'node:dgram'

const network = sugar(dgram, events, worker)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
