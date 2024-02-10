import api from '../stream-relay/api.js'
import { Cache, Packet, sha256, Encryption, NAT } from '../stream-relay/index.js'
import events from 'events'
import dgram from 'dgram'
import worker from 'node:worker_threads'

const network = api(dgram, events, worker)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
