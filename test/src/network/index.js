import test from 'socket:test'
import { isIPv4 } from 'socket:ip'
import { network, Encryption } from 'socket:network'

test('basic network constructor', async t => {
  const sharedKey = await Encryption.createSharedKey('TEST')
  const clusterId = await Encryption.createClusterId('TEST')
  const peerId = await Encryption.createId()
  const signingKeys = await Encryption.createKeyPair()

  const options = {
    clusterId,
    peerId,
    signingKeys
  }

  const socket = await network(options)

  await new Promise((resolve, reject) => {
    socket.on('#ready', info => {
      t.ok(isIPv4(info.address), 'got an ipv4 address')
      t.ok(!isNaN(info.port), 'got a valid port')
      t.equal(Buffer.from(info.peerId, 'hex').length, 32, 'valid peerid')
      resolve()
    })
    socket.on('#error', reject)
  })
})
