const Tonic = require('@operatortc/tonic')
const Components = require('@operatortc/components')

Components(Tonic)

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
  constructor () {
    super()

    window.addEventListener('server', event => {
      this.log(event.detail)
    })

    window.addEventListener('socket', async (event) => {
      this.log(event.detail)
    })

    this.state = {
      serverStatus: 'NOT READY'
    }
  }

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

  log (...args) {
    this.querySelector('#output').value +=
      `\n${JSON.stringify(args, null, 2)}`
  }

  async touchend (e) {
    const el = Tonic.match(e.target, '[data-event]')
    if (!el) return

    const { event } = el.dataset

    if (event === 'listen') {
      this.log('try to listen')

      const { err, data } = await window.system.listen({
        port: 8222
      })

      const { err: errIP, data: dataIP } = await window.system.getIP({})

      if (errIP) {
        this.state.ip = errIP || 'Unable to get ip'
      } else {
        this.state.host = dataIP
      }

      this.state.serverStatus = err || data
      this.reRender()
      return
    }

    if (event === 'connect') {
      const { err, data } = await window.system.connect({
        port: 8222,
        address: '192.168.13.235'
      })

      clearInterval(this.sendInterval)

      this.sendInterval = setInterval(async () => {
        const params = {
          id: data.id,
          message: `The time is ${Date.now()}.`
        }

        const {
          err: errSend,
          data: dataSend
        } = await window.system.send(params)

        this.log(errSend || dataSend)
      }, 1024)

      this.log(err)
      this.log(data)
    }
  }

  async input (e) {
    const el = Tonic.match(e.target, 'tonic-input')
    if (!el) return

    const elResponse = document.querySelector('#response')
    elResponse.value = el.value
  }

  async connected () {
    window.system.receiveData({ id: 0 }, (err, data) => {
      this.querySelector('#output').value += `\n${JSON.stringify(err || data)}`
    })
  }

  async render () {
    const serverStatus = this.state.serverStatus

    return this.html`
      <app-header>
      </app-header>

      <a href="bad-text">BAD ANCHOR</a>
      <a href="https://example.com">DANGER</a>

      <div class="grid">
        <tonic-input id="send" label="send">
        </tonic-input>

        <tonic-input id="response" label="recieve" readonly="true">
        </tonic-input>

        <tonic-button data-event="listen" id="start">Listen</tonic-button>
        <tonic-button data-event="connect" id="start">Connect</tonic-button>
      </div>

      <tonic-toaster-inline
        type="${serverStatus === 'READY' ? 'success' : 'warning'}"
        title="Server Status"
        id="status">
        The server is ${serverStatus}.
      </tonic-toaster-inline>

      <tonic-textarea
        id="output"
        label="output"
        rows="8"
        resize="none"
      >${JSON.stringify(this.state, null, 2)}</tonic-textarea>
    `
  }
}

window.onload = async () => {
  console.log('started', window.process)
  Tonic.add(AppContainer)
}

