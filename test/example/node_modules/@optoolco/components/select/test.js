const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="select">
  <h2>Select</h2>

  <!-- Default select -->
  <div class="test-container">
    <span>Default Select</span>
    <tonic-select id='default-select'>
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Empty select -->
  <div class="test-container">
    <span>Empty Select</span>
    <tonic-select></tonic-select>
  </div>

  <!-- Select w/ id -->
  <div class="test-container">
    <span>id="select-id"</span>
    <tonic-select id="select-id">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select w/ name -->
  <div class="test-container">
    <span>name="select-name"</span>
    <tonic-select name="select-name">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Required select -->
  <div class="test-container">
    <span>required="true"</span>
    <tonic-select required="true">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Non-required select -->
  <div class="test-container">
    <span>required="false"</span>
    <tonic-select required="false">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Disabled select -->
  <div class="test-container">
    <span>disabled="true"</span>
    <tonic-select disabled="true">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Non-disabled select -->
  <div class="test-container">
    <span>disabled="false"</span>
    <tonic-select disabled="false">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with label -->
  <div class="test-container">
    <span>label="Label"</span>
    <tonic-select label="Label">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with width % -->
  <div class="test-container">
    <span>width="100%"</span>
    <tonic-select width="100%">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with height -->
  <div class="test-container">
    <span>height="60px"</span>
    <tonic-select height="60px">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with radius -->
  <div class="test-container">
    <span>radius="8px"</span>
    <tonic-select radius="8px">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with value -->
  <div class="test-container">
    <span>value="c"</span>
    <tonic-select value="c">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with theme light -->
  <div class="test-container">
    <span>theme="light"</span>
    <tonic-select theme="light">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Select with theme dark -->
  <div class="test-container flex-half dark">
    <span>theme="dark"</span>
    <tonic-select theme="dark">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
    </tonic-select>
  </div>

  <!-- Multiple select -->
  <div class="test-container">
    <span>multiple="true"</span>
    <tonic-select multiple="true">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
      <option value="d">Option D</option>
      <option value="e">Option E</option>
    </tonic-select>
  </div>

  <!-- Multiple select, width % -->
  <div class="test-container">
    <span>multiple="true", width="100%"</span>
    <tonic-select multiple="true" width="100%">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
      <option value="d">Option D</option>
      <option value="e">Option E</option>
    </tonic-select>
  </div>

  <!-- Multiple select, size -->
  <div class="test-container">
    <span>multiple="true", size="3"</span>
    <tonic-select multiple="true" size="3">
      <option value="a">Option A</option>
      <option value="b">Option B</option>
      <option value="c">Option C</option>
      <option value="d">Option D</option>
      <option value="e">Option E</option>
    </tonic-select>
  </div>

</section>
`)

// TODO: write tests
tape('test a select', t => {
  const select = qs('#default-select')

  t.ok(select.querySelector('select'))
  t.equal(
    select.querySelectorAll('select option').length,
    3
  )

  const styles = window.getComputedStyle(
    select.querySelector('select')
  )
  t.equal(styles.color, 'rgb(51, 51, 51)')

  t.end()
})
