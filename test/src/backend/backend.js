import socket from './socket-node.js'

socket.on('character2', async (value) => {
  await socket.send({
    window: 0,
    event: 'character2',
    value
  })
})

await socket.send({
  window: 0,
  event: 'backend:ready'
})
await socket.send({
  window: 0,
  event: 'character',
  value: { firstname: 'Morty', secondname: 'Smith' }
})
