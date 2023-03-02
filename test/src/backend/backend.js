import socket from './socket-node.js'

socket.on('20 minutes adventure', async (value) => {
  await socket.send({
    window: 0,
    event: '20 minutes adventure',
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
