const Tonic = require('@operatortc/tonic')
const Components = require('@operatortc/components')

Components(Tonic)

//
// A keybinding example... if keyup is ctrl+q, quit the app
//
window.addEventListener('click', async event => {
  const el = Tonic.match(event.target, '#externalLink')
  if (!el) return

  event.preventDefault()

  // await system.openExternal(el.props.url)
})

window.addEventListener('touchstart', event => {
  const el = Tonic.match(event.target, 'data-event')
  if (!el) return event.preventDefault()
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
      <h2>${this.props.message}</h2>
    `
  }
}

Tonic.add(AppHeader)

class AppContainer extends Tonic {
  static stylesheet () {
    return `
      app-container {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        overflow: hidden;
        padding: 20px;
      }

      app-container h1 {
        font-size: 24px;
        font-weight: 900;
        margin: 0;
        padding: 0;
      }

      app-container img {
        width: 100%;
        height: auto;
      }
    `
  }

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
      this.querySelector('#opened').value = await system.dialog({})
    }

    if (event === 'save') {
      this.querySelector('#opened').value = await system.dialog({
        type: 'save',
        defaultPath: 'test.txt'
      })
    }
  }

  async input (e) {
    const el = Tonic.match(e.target, 'tonic-input')
    if (!el) return


    const elResponse = document.querySelector('#response')
    elResponse.value = el.value

    console.log(el.value)

    /* try {
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

    return // system.setTitle({ e.target.value) */
  }

  async render () {
    // const settings = await system.getSettings()
    // console.log(settings)

    return this.html`
      <img src="./images/monogram-large.png" />
      <app-header>
      </app-header>

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

      <tonic-button id="externalLink" url="https://example.com">External</tonic-button>
    `
  }
}

window.onload = async () => {
  console.log('started', window.process)
  Tonic.add(AppContainer)
}
