import main from './opkit.js'

let counter = 0

console.log('started')

setTimeout(() => {
  main.setTitle('Hello, World!')
  main.setSize({ height: 500, width: 750 })
}, 2048)

main.receive(async data => {
  return {
    received: data,
    counter: counter++
  }
})

setInterval(() => {
  counter++
 
  // send an odd sized message
  const size = Math.floor(Math.random() * 1e3)
  const data = new Array(size).fill(0)

  main.send({ sending: data.join(''), counter })
}, 5090) // send at some interval

process.on('beforeExit', () => {
  console.log('exiting')
})
