const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="range">
  <h2>Range</h2>

  <!-- Range Default -->
  <div class="test-container">
    <span>Default Range Input</span>
    <tonic-range id="range-default"></tonic-range>
  </div>

  <!-- Range w/ id -->
  <div class="test-container">
    <span>id="range-id"</span>
    <tonic-range id="range-id"></tonic-range>
  </div>

  <!-- Range w/ name -->
  <div class="test-container">
    <span>name="range-name"</span>
    <tonic-range id="range-name" name="range-name"></tonic-range>
  </div>

  <!-- Disabled range -->
  <div class="test-container">
    <span>disabled="true"</span>
    <tonic-range id="range-disabled-true" disabled="true"></tonic-range>
  </div>

  <!-- Non-disabled range -->
  <div class="test-container">
    <span>disabled="false"</span>
    <tonic-range id="range-disabled-false" disabled="false"></tonic-range>
  </div>

  <!-- Range w/ Width px -->
  <div class="test-container">
    <span>width="100px"</span>
    <tonic-range id="range-width" width="100px"></tonic-range>
  </div>

  <!-- Range w/ Width % -->
  <div class="test-container">
    <span>width="100%"</span>
    <tonic-range id="range-width-full" width="100%"></tonic-range>
  </div>

  <!-- Range w/ Height px -->
  <div class="test-container">
    <span class="fail">height="10px"</span>
    <tonic-range id="range-height" width="10px"></tonic-range>
  </div>

  <!-- Range w/ Height % -->
  <div class="test-container">
    <span>height="100%"</span>
    <tonic-range id="range-height-full" height="100%"></tonic-range>
  </div>

  <!-- Range w/ Radius -->
  <div class="test-container">
    <span>radius="2px"</span>
    <tonic-range id="range-radius" height="2px"></tonic-range>
  </div>

  <!-- Range w/ min -->
  <div class="test-container">
    <span>min="15"</span>
    <tonic-range id="range-min" label="%i%" min="15"></tonic-range>
  </div>

  <!-- Range w/ max -->
  <div class="test-container">
    <span>max="120"</span>
    <tonic-range id="range-max" label="%i%" max="120"></tonic-range>
  </div>

  <!-- Range w/ step -->
  <div class="test-container">
    <span>step="25"</span>
    <tonic-range id="range-step" label="%i%" step="25"></tonic-range>
  </div>

  <!-- Range w/ label -->
  <div class="test-container">
    <span>label="%i%"</span>
    <tonic-range id="range-label" label="%i%"></tonic-range>
  </div>

  <!-- Range w/ thumb color -->
  <div class="test-container">
    <span>thumb-color="red"</span>
    <tonic-range id="range-thumb-color" thumb-color="red"></tonic-range>
  </div>

  <!-- Range w/ thumb radius -->
  <div class="test-container">
    <span>thumb-radius="0px"</span>
    <tonic-range id="range-thumb-radius" thumb-radius="0px"></tonic-range>
  </div>

  <!-- Range w/ value -->
  <div class="test-container">
    <span>value="90"</span>
    <tonic-range id="range-thumb-value" value="90"></tonic-range>
  </div>

  <!-- Range w/ value (js) -->
  <div class="test-container">
    <span>js (value = 15)</span>
    <tonic-range id="range-thumb-value-js"></tonic-range>
  </div>

  <!-- Range w/ theme light -->
  <div class="test-container">
    <span>theme="light"</span>
    <tonic-range id="range-theme-light" theme="light"></tonic-range>
  </div>

  <!-- Range w/ theme dark -->
  <div class="test-container flex-half dark">
    <span>theme="dark"</span>
    <tonic-range id="range-theme-dark" theme="dark"></tonic-range>
  </div>

</section>
`)

const rangeValue = document.getElementById('range-thumb-value-js')
rangeValue.value = 15

// TODO: convert to tape tests
tape('test a range elem', t => {
  const range = qs('#range-default')
  const range2 = qs('#range-thumb-value')

  t.equal(range.value, '50')
  t.equal(range2.value, '90')

  const input = qs('input', range)
  t.ok(input)

  const styles = window.getComputedStyle(input)
  t.equal(styles.height, '4px')

  const input2 = qs('input', range2)
  const styles2 = window.getComputedStyle(input2)
  t.equal(styles2.backgroundSize, '90% 100%')

  t.end()
})
