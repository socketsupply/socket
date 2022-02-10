const Tonic = require('@socketsupply/tonic')
const Components = require('@socketsupply/components')

const { createServer } = require('net')
const { readFile } = require('fs')

Components(Tonic)

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

    // data is a firehose of information.
    window.addEventListener('data', event => {
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
    const textarea = this.querySelector('#output')
    textarea.value += `\n${JSON.stringify(args, null, 2)}`
    textarea.scrollTop = textarea.scrollHeight
  }

  async touchend (e) {
    const el = Tonic.match(e.target, '[data-event]')
    if (!el) return

    const { event } = el.dataset

    if (event === 'listen') {
      this.log('listener starting')

      const { err, data } = await system.getNetworkInterfaces({});

      if (err) {
        this.log('could not get network interfaces')
        return
      }

      this.log('network interfaces', data)

      const server = createServer({ port: 9200 })

      server.on('connection', socket => {
        socket.on('data', data => {
          this.log('got data', data)
          socket.write(data)
        })
      })

      server.on('listening', data => {
        this.log('listening', data)
        this.state.serverStatus = 'READY'
        this.reRender()
      })
    }

    if (event === 'connect') {
      //
      // Returns a uint64 id for calling `net.send(id, message)`
      //
      this.log('attempt to connect')

      const socket = await net.createConnection({
        port: 9200,
        address: '192.168.13.235'
      })

      if (err) {
        this.log(err)
        return
      }

      socket.on('connect', () => {
        clearInterval(this.sendInterval)

        this.sendInterval = setInterval(async () => {
          this.log('attempt to send')

          const { err } = await socket.write(`The time is ${Date.now()}.`)
          this.log('sent', err)
        }, 1024)

        this.log(err || data)
      })

      socket.on('end', () => {
        clearInterval(this.sendInterval)
      })
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
      <a href="https://example.com">EXTERNAL LINK</a>

      <div class="grid">
        <tonic-input id="send" label="send">
        </tonic-input>

        <tonic-input id="response" label="recieve" readonly="true">
        </tonic-input>

        <tonic-button width="100%" data-event="listen" id="start">Listen</tonic-button>
        <tonic-button width="100%" data-event="connect" id="start">Connect</tonic-button>
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
  // init the main component
  Tonic.add(AppContainer)
}

