const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="router">
  <h2>Router</h2>

  <div class="test-container">
    <span>Router examples</span>
    <tonic-select
      id="tonic-router-select"
      value="/"
      label="Select a URL">
      <option value="/">/</option>
      <option value="/bar/100">/bar/100</option>
      <option value="/beepboop">/beepboop</option>
    </tonic-select>

    <!-- Router w/ path -->
    <tonic-router id="page1" path="/">
      <i>Hello, World</i>
    </tonic-router>

    <!-- Router w/ id -->
    <tonic-router id="page2" path="/bar/:number">
      <b>number</b> prop has the value <b id="page2-number"></b>.
    </tonic-router>

    <!-- Router w/ none -->
    <tonic-router id="404">
      404
    </tonic-router>
  </div>

</section>
`)

const select = document.getElementById('tonic-router-select')
const page2 = document.getElementById('page2')

select.addEventListener('change', e => {
  window.history.pushState({}, '', select.value)
})

page2.addEventListener('match', () => {
  const { number } = page2.state
  const el = document.getElementById('page2-number')
  if (!el) return
  el.textContent = number
})

// TODO: convert to tape tests
