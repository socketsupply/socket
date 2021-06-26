import system from './system.js'
import path from 'path'
import fs from 'fs'

let counter = 0

async function main () {

  //
  // Show one of the windows
  //
  await system.show({ window: 0 })

  //
  // Navigate from the current location
  //
  const file = path.join(path.dirname(process.argv[1]), 'index.html')
  await system.navigate({ window: 0, value: `file://${file}` })

  //
  // Set the title of a window
  //
  // await system.setTitle({ window: 0, value: 'Hello' })

  //
  // A template to set the window's menu
  //
  /* const menu = `
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
  
  await system.setMenu({ window: 0, value: menu }) */

  //
  // Handling inbound messages and returning responses
  //
  system.receive = async data => {
    fs.appendFileSync('>>>>>>>>', data)
    return {
      received: data,
      counter: counter++
    }
  }

  //
  // Handling arbitrary fire-and-forget messages
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