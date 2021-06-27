import system from './system.js'
import path from 'path'

let counter = 0

async function main () {
  //
  // TODO (@heapwolf): need to test rejected promises / failure modes.
  //

  //
  // ## Example
  // Show one of the windows
  //
  await system.show({ window: 0 })

  //
  // ## Example
  // Navigate from the current location
  //
  const file = path.join(path.dirname(process.argv[1]), 'index.html')
  await system.navigate({ window: 0, value: `file://${file}` })

  //
  // ## Example
  // Set the title of a window
  //
  await system.setTitle({ window: 0, value: 'Hello' })

  //
  // ## Example
  // A template to set the window's menu
  //
  const menu = `
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
  `

  await system.setMenu({ window: 0, value: menu })

  //
  // ## Example
  // Handling inbound messages and returning responses.
  // This example is basically an "echo" server...
  //
  system.receive = async (command, value) => {
    return {
      received: value,
      command,
      counter: counter++
    }
  }

  //
  // ## Example
  // Sending arbitrary fire-and-forget messages to the render process.
  //
  setInterval(() => {
    counter++

    // send an odd sized message that can be validated
    // on the front end.
    const size = Math.floor(Math.random() * 1e3)
    const data = new Array(size).fill(0)

    //
    // we need to specify which window, the event name
    // that will be listening for this data, and the value
    // which can be plain old json data.
    //
    system.send({
      index: 0,
      event: 'data',
      value: {
        sending: data.join(''),
        counter,
        size
      }
    })
  }, 1024) // send at some interval

  process.on('beforeExit', () => {
    console.log('exiting')
  })
}

main()
