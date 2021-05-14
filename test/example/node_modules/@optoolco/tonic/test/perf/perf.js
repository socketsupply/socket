const Benchmark = require('benchmark')
const Tonic = require('../../dist/tonic.min.js')

window.Benchmark = Benchmark
const suite = new Benchmark.Suite()

class XHello extends Tonic {
  render () {
    return `<h1>${this.props.message}</h1>`
  }
}

class XApp extends Tonic {
  render () {
    return this.html`
      <x-hello message="${Math.random()}">
      </x-hello>
    `
  }
}

document.body.innerHTML = `
  <script></script>
  <x-app></x-app>
`

Tonic.add(XHello)
Tonic.add(XApp)

document.addEventListener('DOMContentLoaded', () => {
  const app = document.querySelector('x-app')
  const hello = document.querySelector('x-hello')

  suite
    .add('re-render a single component', () => {
      hello.reRender({ message: Math.random() })
    })
    .add('re-render a hierarchy component', () => {
      app.reRender()
    })
    .on('cycle', (event) => {
      console.log(String(event.target))
    })
    .on('complete', function () {
      console.log('done')
    })
    .run()
})
