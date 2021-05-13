import ipc from './ipc.js'

let counter = 0

console.log('started')

ipc.receive(async data => {
  return {
    received: data,
    counter: counter++
  }
})

let isBreakTime = false
let breakCounter = 0

setInterval(() => {
  counter++

  ipc.send({ sending: Date.now(), counter })
}, 2048)

process.on('beforeExit', () => {
  console.log('exiting')
})
