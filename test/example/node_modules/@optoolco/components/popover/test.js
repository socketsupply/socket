const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="popover">
  <h2>Popover</h2>

  <!-- Popover Default -->
  <div class="test-container">
    <span>Default Popover</span>
    <tonic-button id="tonic-popover-default-button">
      Open Popover
    </tonic-button>
  </div>

  <tonic-popover
    id="tonic-popover-default"
    width="175px"
    for="tonic-popover-default-button">
    <div>Item 1</div>
    <div>Item 2</div>
    <div>Item 3</div>
  </tonic-popover>

</section>
`)

//
// Panel Default
//
const popover = document.getElementById('tonic-popover-default')
popover.addEventListener('show', event => {
  document.body.addEventListener('click', e => {
    popover.hide()
  }, { once: true })
})

// TODO: write tests for popover.
tape('opening popover', async t => {
  const container = qs('#popover')
  const popover = qs('#tonic-popover-default', container)
  const button = qs('#tonic-popover-default-button', container)

  t.ok(popover)
  t.ok(button)

  const popoverCont = qs('.tonic--popover', popover)
  t.ok(popoverCont)

  const divs = popoverCont.querySelectorAll('div')
  t.equal(divs.length, 3)

  const styles = window.getComputedStyle(divs[0])
  t.equal(styles.visibility, 'hidden')

  button.querySelector('button').click()
  await sleep(512)

  const styles2 = window.getComputedStyle(divs[0])
  t.equal(styles2.visibility, 'visible')

  t.end()
})

function sleep (ms) {
  return new Promise(resolve => {
    setTimeout(resolve, ms)
  })
}
