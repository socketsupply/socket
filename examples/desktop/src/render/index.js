const Tonic = require('@operatortc/tonic')
const Components = require('@operatortc/components')

Components(Tonic)

window.addEventListener('contextmenu', e => {
  if (!process.debug) {
    e.preventDefault()
  }
})

//
// Menu item selection example... do whatever, await an ipc call, etc.
//
window.addEventListener('menuItemSelected', event => {
  const { title, parent } = event.detail

  if (title) {
    document.querySelector('#menu-selection').value = event.detail.title

    if (title.toLowerCase() === 'quit') {
      system.exit(0)
    }

    switch (parent.toLowerCase()) {
      case 'edit': {
        switch (title.toLowerCase()) {
          case 'cut': {
            document.execCommand('cut')
            break
          }

          case 'copy': {
            document.execCommand('copy')
            break
          }

          case 'paste': {
            document.execCommand('paste')
            break
          }

          case 'select all': {
            if (document.activeElement && document.activeElement.select) {
              document.activeElement.select()
            } else {
              document.getSelection().extend(document.activeElement)
            }
            break
          }

          case 'delete': {
            if (document.activeElement && document.activeElement.value) {
              if ('selectionStart' in document.activeElement && 'selectionEnd' in document.activeElement) {
                const { selectionStart, selectionEnd, value } = document.activeElement
                document.activeElement.value = value.slice(0, selectionStart).concat(value.slice(selectionEnd))
              }
            }
            break
          }
        }
      }
    }
  }
})

//
// A keybinding example... if keyup is ctrl+q, quit the app
//
window.addEventListener('keyup', async event => {
  if (event.ctrlKey && event.key === 'q') {
    system.exit(0)
  }
})

//
// A keybinding example... if keyup is ctrl+q, quit the app
//
window.addEventListener('click', async event => {
  const el = Tonic.match(event.target, '#externalLink')
  if (!el) return

  event.preventDefault()

  await system.openExternal(el.props.url)
})

//
// Receive arbitrary/non-request-response data from the main process.
//
window.addEventListener('data', event => {
  if (event.detail.env) {
    console.log(event)
    return
  }

  if (event.detail.size !== event.detail.sending.length) {
    throw new Error('Not aligned: detail size not accurate')
  } else {
    console.log(`received ${event.detail.size} characters`)
  }

  AppContainer.setHeader(`${event.detail.counter} messages received`)
})

//
// Create some arbitrary components with our nifty component framework.
//
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
    const el = Tonic.match(e.target, '[data-event]')
    if (!el) return

    const { event } = el.dataset

    if (event === 'open') {
      this.querySelector('#output').value = await system.dialog({
        allowDirs: true,
        allowFiles: false
      })
    }

    if (event === 'save') {
      this.querySelector('#output').value = await system.dialog({
        type: 'save',
        defaultPath: 'test.txt'
      })
    }

    if (event === 'restart') {
      system.send({
        restart: true
      })
    }

    if (event === 'showInspector') {
      system.showInspector()
    }
  }

  async input (e) {
    const el = Tonic.match(e.target, 'tonic-input')
    if (!el) return

    let response

    try {
      //
      // request-response (can send any arbitrary parameters)
      //
      const value = { input: e.target.value }
      response = await system.send(value)
    } catch (err) {
      console.log(err.message)
    }

    this.querySelector('#response').value =
      response.received.input

    return // system.setTitle({ e.target.value)
  }

  async dragstart (e) {
    var dataList = e.dataTransfer.items
    dataList.add('https://foo.com https://x.com', 'text/plain')
  }

  async contextmenu (e) {
    const el = Tonic.match(e.target, '.context-menu')
    if (!el) return

    e.preventDefault()

    const choice = await system.setContextMenu({
      'Download': 'd',
      'Wizard': 'w',
      '---': '',
      'Inspect': 'i'
    })

    if (choice.title === 'Inspect') {
      // system.inspect()
    }

    if (choice.title) {
      document.querySelector('#menu-selection').value = choice.title
    }
  }

  async render () {
    // const settings = await system.getSettings()
    // console.log(settings)

    return this.html`
      <app-header>
      </app-header>
      <!-- <code>{JSON.stringify(window.process)}</code> -->
      <a href="bad-text">BAD ANCHOR</a>
      <a href="https://example.com">FACEBOOK</a>

      <div class="grid">
        <tonic-input id="send" label="send">
        </tonic-input>

        <tonic-input id="response" label="recieve" readonly="true">
        </tonic-input>
      </div>

      <tonic-button data-event="open" id="open">Open</tonic-button>
      <tonic-button data-event="save" id="save">Save</tonic-button>
      <tonic-button data-event="restart" id="restart">Restart</tonic-button>

      <tonic-textarea id="output" label="output files/dirs"></tonic-textarea>

      <div class="grid">
        <tonic-input id="menu-selection" readonly="true" label="menu selection">
        </tonic-input>

        <div class="context-menu" draggable="true">
          Context Menu Enabled Area
        </div>
      </div>

      <tonic-button id="externalLink" url="https://example.com">External</tonic-button>
      <!-- tonic-button data-event="showInspector">Inspect</tonic-button -->

      <a id="dd" draggable="true" href="file:///Users/paolofragomeni/projects/optoolco/opkit/test/example/dist/Operator.app/Contents/Resources/log.txt">Draggable</a>
      <a id="dl" href="file:///Users/paolofragomeni/projects/optoolco/opkit/src/render.html" download>Download</a>
    `
  }
}

window.onload = async () => {
  Tonic.add(AppContainer)

  //
  // If the user drops files, show them in the output area. It's the user's
  // responsibility to decide what to do with the file paths, batching, etc.
  //
  window.addEventListener('drop', (e) => {
    const { x, y, path } = e.detail

    //
    // We can get the element at the drop point from x and y.
    //
    const el = document.elementFromPoint(x, y)

    if (!el) throw new Error('No element found at drop point')
    console.log(el)

    document.querySelector('#output').value += path + '\n'
  });
}
