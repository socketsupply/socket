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
 
  // send an odd sized message
  const size = Math.floor(Math.random() * 1e3)
  const data = new Array(size).fill(0)

  ipc.send({ sending: data.join(''), counter })
}, 5090) // send at some interval

process.on('beforeExit', () => {
  console.log('exiting')
})
