const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const Tonic = require('@optoolco/tonic')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

class PanelInner extends Tonic {
  async click (e) {
    if (e.target.value === 'close') {
      return this.hide()
    }
  }

  render () {
    return this.html`
      <header>Panel Example</header>

      <main>
        <h3>${this.props.title || 'Hello'}
      </main>

      <footer>
        <tonic-button value="close">Close</tonic-button>
      </footer>
    `
  }
}

Tonic.add(PanelInner)

document.body.appendChild(html`
<section id="panel">
  <h2>Panel</h2>

  <!-- Panel Default -->
  <div class="test-container">
    <span>Default Panel</span>
    <tonic-button id="example-panel-default-button">
      Open Panel
    </tonic-button>
  </div>

  <tonic-panel id="example-panel-default">
    <panel-inner id="example-panel-inner-default">
    </panel-inner>
  </tonic-panel>

  <!-- Panel w/ Position Left -->
  <div class="test-container">
    <span>position="right"</span>
    <tonic-button id="example-panel-position-button">
      Open Panel
    </tonic-button>
  </div>

  <tonic-panel id="example-panel-position" position="left">
    <panel-inner id="example-panel-inner-position">
    </panel-inner>
  </tonic-panel>

</section>
`)

//
// Panel Default
//
const panelDefaultButton = document.getElementById('example-panel-default-button')
const panelDefault = document.getElementById('example-panel-default')

panelDefaultButton.addEventListener('click', e => panelDefault.show())

//
// Panel w/ Position Left
//
const panelPositionButton = document.getElementById('example-panel-position-button')
const panelPosition = document.getElementById('example-panel-position')

panelPositionButton.addEventListener('click', e => panelPosition.show())

tape('opening a panel', async t => {
  const container = qs('#example-panel-default')
  const overlay = qs('.tonic--overlay')
  const main = qs('main', container)
  const h3 = qs('h3', main)

  t.ok(container)
  t.ok(overlay)
  t.ok(main)
  t.ok(h3)

  t.equal(h3.textContent.trim(), 'Hello')

  t.ok(container.hasAttribute('hidden'))

  await container.show()

  t.equal(container.hasAttribute('hidden'), false, 'is not hidden')

  await container.hide()

  t.ok(container.hasAttribute('hidden'))

  t.end()
})
