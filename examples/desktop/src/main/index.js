const path = require('path')
const assert = require('assert')
const system = require('@socketsupply/op-node')

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
  await system.show({ window: 1 })

  //
  // ## Example
  // Navigate from the current location
  //
  const resourcesDirectory = path.dirname(process.argv[1])
  const file = path.join(resourcesDirectory, 'index.html')
  await system.navigate({ window: 0, value: `file://${file}` })
  await system.navigate({ window: 1, value: `file://${file}` })

  // await system.setSize({ window: 0, width: 200, height: 200 })

  const size = await system.getScreenSize()

  assert(size.width, 'screen has width')
  assert(size.height, 'screen has width')

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
      ---
      Preferences...: , + Meta
      ---
      Hide: h + Meta
      Hide Others: h + CommandOrControl + Meta
      ---
      Quit: q + CommandOrControl
    ;

    Edit:
      Cut: x + CommandOrControl
      Copy: c + CommandOrControl
      Paste: v + CommandOrControl
      Delete: _
      Select All: a + CommandOrControl
    ;

    Foo:
      Bazz: z + Meta
      ---
      Quxx: e + Meta + Alt
    ;

    Other:
      Another Test: t + Alt
      Beep: T + Meta
    ;
  `

  await system.setMenu({ window: 0, value: menu })

  //
  // ## Example
  // Handling inbound messages and returning responses.
  // This example is basically an "echo" server...
  //
  system.receive = async (command, value) => {
    if (value && value.restart) {
      await system.restart()
    }

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
  let i = 3
  setInterval(() => {
    counter++

    // send an odd sized message that can be validated
    // on the front end.
    // const size = Math.floor(Math.random() * 14e5)
    const size = 14
    const data = new Array(size).fill(0)

    //
    // we need to specify which window, the event name
    // that will be listening for this data, and the value
    // which can be plain old json data.
    //
    if (i-- <= 0) return

    system.send({
      window: 0,
      event: 'data',
      value: {
        sending: data.join(''),
        counter,
        size
      }
    })
  }, 16 * 100) // send at some interval
}

main()
