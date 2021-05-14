const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="toggle">
  <h2>Toggle</h2>

  <div id="toggle-1" class="test-container">
    <span>Default Toggle</span>
    <tonic-toggle id="toggle-default"></tonic-toggle>
  </div>

  <div id="toggle-2" class="test-container">
    <span>tab-index="0"</span>
    <tonic-toggle id="toggle-tabindex" tabindex="0"></tonic-toggle>
  </div>

  <div id="toggle-3" class="test-container">
    <span>label="label"</span>
    <tonic-toggle label="label" id="toggle-label"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>label="label"</span>
    <tonic-toggle id="toggle-2" label="label"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>id="toggle-3"</span>
    <tonic-toggle id="toggle-3"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>name="toggle-name"</span>
    <tonic-toggle id="toggle-4" name="toggle-name"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>disabled="true"</span>
    <tonic-toggle id="toggle-5" disabled="true"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>disabled="false"</span>
    <tonic-toggle id="toggle-6" disabled="false"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>checked="true"</span>
    <tonic-toggle id="toggle-7" checked="true"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>checked="false"</span>
    <tonic-toggle id="toggle-8" checked="false"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>checked="true", disabled="true"</span>
    <tonic-toggle id="toggle-9" checked="true" disabled="true"></tonic-toggle>
  </div>

  <div class="test-container">
    <span>theme="light"</span>
    <tonic-toggle id="toggle-10" theme="light"></tonic-toggle>
  </div>

  <div class="test-container flex-half dark">
    <span>theme="dark"</span>
    <tonic-toggle id="toggle-11" theme="dark"></tonic-toggle>
  </div>

  <div class="test-container flex-half dark">
    <span>theme="dark", disabled="true"</span>
    <tonic-toggle id="toggle-12" theme="dark" disabled="true"></tonic-toggle>
  </div>

  <div class="test-container flex-half dark">
    <span>theme="dark", checked="true", disabled="true"</span>
    <tonic-toggle id="toggle-13" theme="dark" checked="true" disabled="true"></tonic-toggle>
  </div>

</section>
`)

tape('{{toggle-1}} default state renders properly', t => {
  const container = qs('#toggle-1')
  const component = qs('tonic-toggle', container)
  const toggleWrapper = qs('.tonic--switch', component)
  const input = qs('input', toggleWrapper)
  const label = qs('label', toggleWrapper)

  t.plan(4)

  t.ok(input, 'the component was constructed with an input')
  t.ok(toggleWrapper, 'the component was constructed with a toggle wrapper')
  t.ok(component.hasAttribute('id'), 'the component has an id')
  t.equal(input.getAttribute('id'), label.getAttribute('for'), 'the input id matches the label')

  t.end()
})

tape('{{toggle-2}} has tabindex attribute', t => {
  const container = qs('#toggle-2')
  const component = qs('tonic-toggle', container)
  const toggleWrapper = qs('.tonic--switch', component)
  const input = qs('input', toggleWrapper)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.hasAttribute('tabindex'), false, 'component does not have a tabindex')
  t.equal(input.hasAttribute('tabindex'), true, 'input has a tabindex')

  t.end()
})
