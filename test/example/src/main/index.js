import window from './window.js'

let counter = 0

async function main () {
  console.log('started')

  await window.setSize({ height: 500, width: 750 })

  await window.setMenu(`
    Operator:
      About Operator: _
      ---: _
      Preferences...: , + Command
      ---: _
      Hide: h
      Hide Others: h + Control, Command
      ---: _
      Quit: q + Command;

    Edit:
      Cut: x
      Copy: c
      Paste: v
      Delete: _
      Select All: a;

    Foo:
      Bazz: z + Command
      ---: _
      Quxx: e + ControlOrCommand, Option;

    Other:
      Another Test: t
      Beep: T + Command
  `)

  await window.setTitle('Hello Operator')

  window.receive(async data => {
    return {
      received: data,
      counter: counter++
    }
  })

  window.send({ env: process.env })

  setInterval(() => {
    counter++

    // send an odd sized message that can be validated
    // on the front end.
    const size = Math.floor(Math.random() * 1e3)
    const data = new Array(size).fill(0)

    window.send({ sending: data.join(''), counter, size })
  }, 512) // send at some interval

  process.on('beforeExit', () => {
    console.log('exiting')
  })

}

main()
