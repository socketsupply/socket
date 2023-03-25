import diagnostics from 'socket:diagnostics'
import { Buffer } from 'socket:buffer'
import dgram from 'socket:dgram'
import path from 'socket:path'
import test from 'socket:test'
import dns from 'socket:dns/promises'
import fs from 'socket:fs/promises'
import os from 'socket:os'

const TMPDIR = `${os.tmpdir()}${path.sep}`
const FIXTURES = /android/i.test(os.platform())
  ? '/data/local/tmp/ssc-socket-test-fixtures/'
  : `${TMPDIR}ssc-socket-test-fixtures${path.sep}`

test('diagnostics - channels - simple', async (t) => {
  const channel = diagnostics.channel('simple')
  t.equal(channel.hasSubscribers, false, 'channel.hasSubscribers === false')

  await new Promise((resolve) => {
    channel.subscribe(function onMessage (message, name, chan) {
      t.equal(typeof message, 'object', 'typeof message === \'object\'')
      t.equal(message.key, 'value', 'message.key === \'value\'')
      t.equal(name, 'simple', 'name === \'simple\'')
      t.equal(chan, channel, 'chan === channel')
      channel.unsubscribe(onMessage)
      t.equal(channel.hasSubscribers, false, 'channel.hasSubscribers === false')
      resolve()
    })

    t.equal(channel.hasSubscribers, true, 'channel.hasSubscribers === true')
    const promise = channel.publish({ key: 'value' })
    t.ok(promise instanceof Promise, 'channel.publish() -> Promise')
    promise.then((status) => {
      t.equal(status, true, '(await channel.publish()) === true')
    })
  })
})

test('diagnostics - channel groups - simple', async (t) => {
  const group = diagnostics.channels.group('diagnostics group')
  const channels = [
    group.channel('b:2'),
    group.channel('b:1'),
    group.channel('a:2'),
    group.channel('a:1')
  ]

  t.equal(group.hasSubscribers, false, 'group.hasSubscribers === false')

  await new Promise((resolve) => {
    let pending = 2
    group.subscribe('a:*', (message, name, chan) => {
      let channelIndex = -1

      if (name.endsWith('a:1')) {
        pending -= 1
        channelIndex = 3
      } else if (name.endsWith('a:2')) {
        pending -= 1
        channelIndex = 2
      }

      t.equal(chan, channels[channelIndex], `${name} channel seen`)
      t.equal(chan.group, group, `${group.name} group set for ${chan.name}`)
      t.equal(message?.hello, 'world', 'message.hello == \'world\'')

      if (pending === 0) {
        resolve()
      }
    })

    group.publish('a:*', { hello: 'world' })
  })
})

test('diagnostics - channels - builtin - udp', async (t) => {
  const pending = []
  const group = diagnostics.channels.group('udp')
  let closed = false

  pending.push(new Promise((resolve) => {
    group.channel('socket').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.socket', '\'udp.socket\' message called')
      t.ok(message.socket, 'message.socket')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('bind').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.bind', 'udp.bind')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('message').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.message', 'udp.message')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('send.start').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.send.start', 'udp.send.start')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('send.end').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.send.end', 'udp.send.end')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    let pending = 2
    group.channel('close').subscribe((message, name, chan) => {
      if (closed) return
      t.equal(name, 'udp.close', 'udp.close')
      if (--pending === 0) {
        closed = true
        resolve()
      }
    })
  }))

  const socket = dgram.createSocket('udp4')
  socket.bind(8888, () => {
    const { address, port } = socket.address()
    const buffer = Buffer.from('hello')
    const sender = dgram.createSocket('udp4')
    sender.send(buffer, 0, buffer.length, port, address, (err) => {
      if (err) t.ifError(err)
      sender.close(() => {
        socket.close()
      })
    })
  })

  await Promise.all(pending)
})

test('diagnostics - channels - builtin - fs', async (t) => {
  const pending = []
  const group = diagnostics.channels.group('fs')

  pending.push(new Promise((resolve) => {
    group.channel('handle').subscribe((message, name, chan) => {
      t.equal(name, 'fs.handle', 'fs.handle message')
      t.ok(message.handle, 'message.handle')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('handle.open').subscribe((message, name, chan) => {
      t.equal(name, 'fs.handle.open', 'fs.handle.open message')
      t.ok(message.handle, 'message.handle')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('handle.close').subscribe((message, name, chan) => {
      t.equal(name, 'fs.handle.close', 'fs.handle.close message')
      t.ok(message.handle, 'message.handle')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('handle.read').subscribe((message, name, chan) => {
      t.equal(name, 'fs.handle.read', 'fs.handle.read message')
      t.equal(message.bytesRead, 4, 'message.bytesRead === 4')
      t.ok(message.handle, 'message.handle')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('handle.write').subscribe((message, name, chan) => {
      t.equal(name, 'fs.handle.write', 'fs.handle.write message')
      t.equal(message.bytesWritten, 4, 'message.bytesWritten === 4')
      t.ok(message.handle, 'message.handle')
      resolve()
    })
  }))

  const handle = await fs.open(FIXTURES + 'file.txt', 'r+')
  const buffer = Buffer.alloc(4)

  await handle.read(buffer, 0, buffer.length, 0)
  await handle.write(buffer, 0, buffer.length, 0)
  await handle.close()
  await Promise.all(pending)
})

test('diagnostics - channels - builtin - dns', async (t) => {
  const pending = []
  const group = diagnostics.channels.group('dns')

  pending.push(new Promise((resolve) => {
    group.channel('lookup.start').subscribe((message, name, chan) => {
      t.equal(typeof message.hostname, 'string', 'message.host is string')
      t.equal(typeof message.family, 'number', 'message.port is number')
      resolve()
    })
  }))

  pending.push(new Promise((resolve) => {
    group.channel('lookup.end').subscribe((message, name, chan) => {
      t.equal(typeof message.hostname, 'string', 'message.host is string')
      t.equal(typeof message.family, 'number', 'message.port is number')
      resolve()
    })
  }))

  await dns.lookup('google.com')
  await Promise.all(pending)
})
