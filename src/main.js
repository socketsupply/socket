//
// Server side program entry point (requires a loop)
//
import ipc from './ipc.js'

let counter = 0

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

  if (isBreakTime) {
    if (breakCounter++ === 10000) {
      isBreakTime = false
      breakCounter = 0
    }
    return
  }

  if (breakCounter++ === 10000) {
    isBreakTime = true
    breakCounter = 0
    return
  }

  const x = Date.now()

  ipc.send({ sending: x, counter })
}, 512)
