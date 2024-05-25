import { Peer } from './index.js'
import { PeerWorkerProxy } from './proxy.js'
import dgram from '../dgram.js'

let proxy
let peer

globalThis.addEventListener('message', ({ data: source }) => {
  if (proxy) return

  const opts = { isWorkerThread: true }

  proxy = new PeerWorkerProxy(opts, source.port, async function ({ data: args }) {
    const {
      prop,
      data,
      seq
    } = args

    switch (prop) {
      case 'create': {
        peer = new Peer(data, dgram)

        peer.onConnecting = (...args) => this.callMainThread('onConnecting', args)
        peer.onConnection = (...args) => this.callMainThread('onConnection', args)
        peer.onDisconnection = (...args) => this.callMainThread('onDisconnection', args)
        peer.onJoin = (...args) => this.callMainThread('onJoin', args)
        peer.onPacket = (...args) => this.callMainThread('onPacket', args)
        peer.onStream = (...args) => this.callMainThread('onStream', args)
        peer.onData = (...args) => this.callMainThread('onData', args)
        peer.onSend = (...args) => this.callMainThread('onSend', args)
        peer.onFirewall = (...args) => this.callMainThread('onFirewall', args)
        peer.onMulticast = (...args) => this.callMainThread('onMulticast', args)
        peer.onJoin = (...args) => this.callMainThread('onJoin', args)
        peer.onSync = (...args) => this.callMainThread('onSync', args)
        peer.onSyncStart = (...args) => this.callMainThread('onSyncStart', args)
        peer.onSyncEnd = (...args) => this.callMainThread('onSyncEnd', args)
        peer.onQuery = (...args) => this.callMainThread('onQuery', args)
        peer.onNat = (...args) => this.callMainThread('onNat', args)
        peer.onState = (...args) => this.callMainThread('onState', args)
        peer.onWarn = (...args) => this.callMainThread('onWarn', args)
        peer.onError = (...args) => this.callMainThread('onError', args)
        peer.onReady = (...args) => this.callMainThread('onReady', args)
        break
      }

      case 'compileCachePredicate': {
        // eslint-disable-next-line
        let predicate = new Function(`return ${data.toString()}`)()
        predicate = predicate.bind(peer)
        peer.cachePredicate = packet => predicate(packet)
        break
      }

      default: {
        if (isNaN(seq) && peer[prop]) {
          peer[prop] = data
          return
        }

        let r

        try {
          if (typeof peer[prop] === 'function') {
            if (Array.isArray(data)) {
              r = await peer[prop](...data)
            } else {
              r = await peer[prop](data)
            }
          } else {
            r = peer[prop]
          }
        } catch (err) {
          console.error(err)
          return this.resolveMainThread(seq, { err })
        }

        this.resolveMainThread(seq, { data: r })
      }
    }
  })
})
