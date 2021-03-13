//
// Client side program entry point
//

const Tonic = require('@optoolco/tonic')
const Components = require('@optoolco/components')

Components(Tonic)

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
    const len = response.split(',').length

    AppContainer.setHeader(`${len} items selected`)
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
      response = await invokeIPC(value)
    } catch (err) {
      console.log(err.message)
    }
    
    this.querySelector('#response').value =
      response.received.input
  }

  render () {
    return this.html`
      <app-header>
      </app-header>

      <tonic-input id="send" label="send">
      </tonic-input>

      <tonic-input id="response" label="recieve">
      </tonic-input>

      <tonic-button id="butt">Open</tonic-button>

      <a href="file:///Users/paolofragomeni/projects/optoolco/opkit/src/render.html" download>Foo</a>
    `
  }
}

window.onload = async () => {
  Tonic.add(AppContainer)

  document.body.addEventListener('drag', (e) => {
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
  })

  // await invokeIPC('onload')
}
