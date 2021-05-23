//
// Client side program entry point
//

const Tonic = require('@optoolco/tonic')
const Components = require('@optoolco/components')

Components(Tonic)

window.addEventListener('menuItemSelected', event => {
  // can then await invokeIPC if so desired
  document.querySelector('#menu-selection').value = event.detail.title
  console.log(event.detail)
})

window.addEventListener('themeChanged', _ => {
  const theme = document.body.getAttribute('theme')
  document.body.setAttribute('theme', theme === 'dark' ? 'light' : 'dark')
})

window.addEventListener('data', event => {
  //
  // Receive arbitrary/non-request-response data.
  //
  console.log('ARBITRARY DATA', event.detail)
  AppContainer.setHeader(`${event.detail.counter} messages received`)
})

class AppHeader extends Tonic {
  render () {
    return this.html`
      <h1>${this.props.message}</h1>
    `
  }
}

Tonic.add(AppHeader)

class AppContainer extends Tonic {
  static setHeader (message) {
    const appHeader = document.querySelector('app-header')
    appHeader.reRender({
      message
    })
  }

  async click (e) {
    const el = Tonic.match(e.target, 'tonic-button')
    if (!el) return   

    const response = await dialog(e.target.value)
    this.querySelector('#opened').value = response.replace(',', '\n')
  }

  async input (e) {
    // clearTimeout(this.bounceInput)
    // this.bounceInput = setTimeout(async () => {
    const el = Tonic.match(e.target, 'tonic-input')
    if (!el) return

    let response

    try {
      //
      // request-response (can send any arbitrary parameters)
      //
      const value = { input: e.target.value }
      response = await window.send(value)
    } catch (err) {
      console.log(err.message)
    }

    this.querySelector('#response').value =
      response.received.input
    // }, 128)
  }

  async contextmenu (e) {
    const el = Tonic.match(e.target, '.context-menu')
    if (!el) return

    e.preventDefault()

    const choice = await contextMenu({
      'Download': 'd',
      'Wizard': 'w',
      'Share': 's'
    })

    document.querySelector('#menu-selection').value = choice.title
  }

  render () {
    return this.html`
      <app-header>
      </app-header>

      <div class="grid">
        <tonic-input id="send" label="send">
        </tonic-input>

        <tonic-input id="response" label="recieve" readonly="true">
        </tonic-input>
      </div>

      <tonic-button id="butt">Open</tonic-button>

      <tonic-textarea id="opened" label="opened files/dirs"></tonic-textarea>


      <div class="grid">
        <tonic-input id="menu-selection" readonly="true" label="menu selection">
        </tonic-input>

        <div class="context-menu" draggable="true">
          Context Menu Enabled Area
        </div>
      </div>

      <a id="dd" draggable="true" href="file:///Users/paolofragomeni/projects/optoolco/opkit/TODO.md" download>Draggable</a>
      <a id="dl" href="file:///Users/paolofragomeni/projects/optoolco/opkit/src/render.html" download>Download</a>
    `
  }
}

window.onload = async () => {
  Tonic.add(AppContainer)

  // https://developer.apple.com/library/archive/documentation/AppleApplications/Conceptual/SafariJSProgTopics/DragAndDrop.html

  dd.addEventListener('dragover', e => {
    e.preventDefault()
    return true
  })

  dd.addEventListener('dragstart', e => {
    return true
  })

  dd.addEventListener('dragend', _ => {
    const data = event.dataTransfer.items
    return true
  })

  /* document.body.addEventListener('drag', (e) => {
    console.log(e)
  })

  document.body.addEventListener('dragover', (e) => {
    console.log(e)
    e.preventDefault()
    return true
  })

  document.body.addEventListener('drop', (e) => {
    console.log(e)
    e.preventDefault()
  })

  document.body.addEventListener('dragenter', (e) => {
    console.log(e)
    e.preventDefault()
    return true
  })

  document.body.addEventListener('dragstart', (e) => {
    console.log(e)
    return true
  })

  document.body.addEventListener('dragend', (e) => {
    console.log(e)
  }) */

  // await invokeIPC('onload')
}
