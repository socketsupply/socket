import test from '@socketsupply/socket/test.js'
import { network, Encryption } from '@socketsupply/socket'

test('network imports', async t => {
  const socket = await network({
    clusterId: await Encryption.createClusterId(),
    signingKeys: await Encryption.createKeyPair(),
    peerId: await Encryption.createId('xxx')
  })

  await new Promise(resolve => {
    socket.on('#ready', () => {
      t.equal(socket.peer.clusterId.length, 32, 'clusterId was a 32 byte buffer')
      t.equal(socket.constructor.name, 'EventEmitter', 'network function returned an event emitter')
      t.ok(true, 'socket became ready')
      resolve()
    })
  })
})
