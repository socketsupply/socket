window.onerror = message => {
  console.error(message, new Error().stack)
}

window.addEventListener('data', event => {
  console.log(JSON.parse(decodeURIComponent(event.detail)))
})

const Tonic = require('@optoolco/tonic')
const Components = require('@optoolco/components')
Components(Tonic)

class AppHeader extends Tonic {
  render () {
    return this.html`
      <h1>Hello</h1>
    `
  }
}

Tonic.add(AppHeader)

class AppContainer extends Tonic {
  async input (e) {
    const el = Tonic.match(e.target, 'tonic-input')
    if (!el) return

    try {
      /* const x = */ await invokeIPC('test', e.target.value)
      // console.log(`to browser: <${x}>`)
    } catch (err) {
      console.log(err.message)
    }
  }

  render () {
    return this.html`
      <app-header></app-header>
      <tonic-input id="x" label="hi"></tonic-input>
    `
  }
}

window.onload = async () => {
  Tonic.add(AppContainer)
  await invokeIPC('loaded')
}
