import api from '../latica/api.js'
import { Cache, Packet, sha256, Encryption, NAT } from '../latica/index.js'
import events from 'node:events'
import dgram from 'node:dgram'

const network = options => api(options, events, dgram)

export { network, Cache, sha256, Encryption, Packet, NAT }
export default network
