const Tonic = require('@socketsupply/tonic')
const Components = require('@socketsupply/components')

Components(Tonic)

let elementDraggingIndicator
let elementUnderDrag
let lastElementUnderDrag

function setBackgroundColor () {
  const styles = getComputedStyle(document.body)
  let hex = styles.getPropertyValue('--tonic-window').slice(1)
  let [red, green = red, blue = green] = hex.match(/\w\w/g).map(s => parseInt(s, 16))

  window.parent.setBackgroundColor({ red, green, blue, alpha: 1 })
}

window.matchMedia("(prefers-color-scheme: dark)")
  .addListener(setBackgroundColor)

window.addEventListener('contextmenu', e => {
  if (!process.debug) {
    e.preventDefault()
  }
})

window.addEventListener('dropout', e => {
  console.log(e.detail)
  // elementDraggingIndicator.style.display = 'none'
  document.body.removeAttribute('dragging')
})

window.addEventListener('dropin', (e) => {
  const { x, y, src } = e.detail
  const el = document.elementFromPoint(x, y)

  if (!el) throw new Error('No element found at drop point')
  console.log(el)

  document.querySelector('#output').value += src + '\n'
})

window.addEventListener('drag', e => {
  if (!elementDraggingIndicator) {
    elementDraggingIndicator = document.querySelector('#drag-indicator')
  }

  const { x, y, inbound } = e.detail
  elementDraggingIndicator.style.display = inbound ? 'none' : 'block'

  elementDraggingIndicator.style.left = `${x+8}px`
  elementDraggingIndicator.style.top = `${y+8}px`
  elementDraggingIndicator.innerText = e.detail.count

  elementUnderDrag = document.elementFromPoint(x, y)

  if (lastElementUnderDrag && lastElementUnderDrag !== elementUnderDrag) {
    lastElementUnderDrag.blur()
  }

  if (elementUnderDrag) {
    lastElementUnderDrag = elementUnderDrag
    elementUnderDrag.focus()
  }

  if (!document.body.hasAttribute('dragging')) {
    document.body.setAttribute('dragging', 'true')
  }
})

window.addEventListener('dragend', () => {
  setTimeout(() => { // be the last event
    document.body.removeAttribute('dragging')
    if (!elementDraggingIndicator) return
    elementDraggingIndicator.style.display = 'none'
  }, 512)
})

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

window.addEventListener('keyup', async event => {
  if (event.ctrlKey && event.key === 'q') {
    system.exit(0)
  }
})

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

  async mouseup (e) {
    // implement your own drag and drop logic
    if (e.target.id === 'drop-demo') {
      console.log('drop')
    }
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
      system.inspect()
    }

    if (choice.title) {
      document.querySelector('#menu-selection').value = choice.title
    }
  }

  async render () {
    return this.html`
      <app-header>
      </app-header>
      <!-- <code>{JSON.stringify(window.process)}</code> -->
      <a href="bad-text">BAD ANCHOR</a>
      <a href="https://example.com">FACEBOOK</a>

      <div class="dragdrop-demo">
        <div
          class="draggable"
          data-macsrc="/Users/paolofragomeni/projects/optoolco/op/README.md;https://optool.co/images/operator-horizontal.svg;/tmp/test.txt"
          data-src="/tmp/test;/tmp/test2.txt"
        >DRAG HERE</div>

        <div
          class="droppable"
          id="drop-demo"
          tabindex="0"
        >DROP HERE</div>
      </div>

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
      <div id="drag-indicator"></div>
    `
  }
}

window.onload = () => {
  Tonic.add(AppContainer)
  setBackgroundColor()
}
