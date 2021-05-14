const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="checkbox">
  <tonic-sprite></tonic-sprite>
  <h2>Checkbox</h2>

  <div id="checkbox-1" class="test-container">
    <span>Default</span>
    <tonic-checkbox
      id="tonic-checkbox">
    </tonic-checkbox>
  </div>

  <div id="checkbox-2" class="test-container">
    <span>label="label"</span>
    <tonic-checkbox
      id="tonic-checkbox-label"
      label="Label">
    </tonic-checkbox>
  </div>

  <div id="checkbox-3" class="test-container">
    <span>checked="true"</span>
    <tonic-checkbox
      id="tonic-checkbox-checked"
      checked="true">
    </tonic-checkbox>
  </div>

  <div id="checkbox-4" class="test-container">
    <span>disabled="true"</span>
    <tonic-checkbox
      id="tonic-checkbox-disabled"
      disabled="true">
    </tonic-checkbox>
  </div>

  <div id="checkbox-5" class="test-container">
    <span>size="30px"</span>
    <tonic-checkbox
      id="tonic-checkbox-size"
      size="30px">
    </tonic-checkbox>
  </div>

  <div id="checkbox-6" class="test-container">
    <span>tabindex="0"</span>
    <tonic-checkbox
      tabindex="0"
      id="tonic-checkbox-tabindex">
    </tonic-checkbox>
  </div>

  <div id="checkbox-6-5" class="test-container">
    <span>title="foo"</span>
    <tonic-checkbox
      title="foo"
      id="tonic-checkbox-title">
    </tonic-checkbox>
  </div>

  <div id="checkbox-7" class="test-container">
    <span>child elements</span>
    <tonic-checkbox
      tabindex="0"
      id="tonic-checkbox-children">
      This is a <a href="https://google.com" target="blank">label</a>!
    </tonic-checkbox>
  </div>

  <!-- need to fix with new tonic-icon method -->

  <!-- <div id="checkbox-6" class="test-container">
    <span>Custom</span>
    <tonic-checkbox
      id="tonic-checkbox-custom"
      icon-on="./sprite.svg#custom_on"
      icon-off="./sprite.svg#custom_off">
    </tonic-checkbox>
  </div>

  <div id="checkbox-7" class="test-container">
    <span>Custom, checked</span>
    <tonic-checkbox
      id="tonic-checkbox-custom-2"
      checked="true"
      icon-on="./sprite.svg#custom_on"
      icon-off="./sprite.svg#custom_off">
    </tonic-checkbox>
  </div> -->

</section>
`)

tape('{{checkbox-1}} has correct default state', t => {
  const container = qs('#checkbox-1')
  const component = qs('tonic-checkbox', container)
  const wrapper = qs('.tonic--checkbox--wrapper', component)
  const input = qs('input[type="checkbox"]', component)
  const icon = qs('.tonic--icon', component)

  const label = qs('tonic-checkbox label', container)
  t.ok(label)

  const styles = window.getComputedStyle(label)
  t.equal(styles.color, 'rgb(51, 51, 51)')

  t.ok(wrapper, 'component constructed with a wrapper')
  t.ok(input, 'component constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.ok(icon, 'component constructed with default icon')
  t.ok(input.checked === false, 'the default checkbox is not checked')

  t.end()
})

tape('{{checkbox-2}} has correct label', t => {
  const container = qs('#checkbox-2')
  const component = qs('tonic-checkbox', container)
  const input = qs('input[type="checkbox"]', component)
  const label = qs('label:not(.tonic--icon)', component)

  t.plan(3)

  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.ok(label, 'component was constructed with a label')
  t.equal(component.getAttribute('label'), label.textContent, 'the label attribute matches the label text')

  t.end()
})

tape('{{checkbox-3}} is checked', t => {
  const container = qs('#checkbox-3')
  const component = qs('tonic-checkbox', container)
  const input = qs('input[type="checkbox"]', component)

  t.plan(3)

  t.ok(input, 'component was constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.ok(input.checked, 'the input is checked')

  t.end()
})

tape('{{checkbox-4}} is disabled', t => {
  const container = qs('#checkbox-4')
  const component = qs('tonic-checkbox', container)
  const input = qs('input[type="checkbox"]', component)

  t.plan(3)

  t.ok(input, 'component was constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.ok(input.hasAttribute('disabled'), 'the input is disabled')

  t.end()
})

tape('{{checkbox-5}} has size attributes', t => {
  const container = qs('#checkbox-5')
  const component = qs('tonic-checkbox', container)
  const icon = qs('label.tonic--icon', component)
  const input = qs('input[type="checkbox"]', component)
  const size = component.getAttribute('size')

  t.plan(5)

  t.ok(input, 'component was constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.ok(component.hasAttribute('size'), 'the component has a size attribute')
  t.ok(icon.style.width === size, 'the width equals the size attribute')
  t.ok(icon.style.height === size, 'the height equals the size attribute')

  t.end()
})

tape('{{checkbox-6}} has Tabindex', t => {
  const container = qs('#checkbox-6')
  const component = qs('tonic-checkbox', container)
  const input = qs('input[type="checkbox"]', component)

  t.plan(4)

  t.ok(input, 'component was constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.equal(component.hasAttribute('tabindex'), false, 'component does not have a tabindex')
  t.equal(input.hasAttribute('tabindex'), true, 'input has a tabindex')

  t.end()
})

tape('{{checkbox-6-5}} has title', t => {
  const container = qs('#checkbox-6-5')
  const component = qs('tonic-checkbox', container)
  const input = qs('input[type="checkbox"]', component)

  t.plan(5)

  t.ok(input, 'component was constructed with an input')
  t.ok(input.hasAttribute('id'), 'input was constructed with an id')
  t.equal(component.hasAttribute('title'), true, 'component has title')
  t.equal(input.hasAttribute('title'), true, 'input has title')
  t.equal(input.getAttribute('title'), 'foo')

  t.end()
})
