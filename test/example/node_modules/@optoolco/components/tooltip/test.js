const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="tooltip">
  <h2>Tooltip</h2>

  <!-- Default tooltip -->
  <div class="test-container">
    <span>Default tooltip</span>
    <span id="tonic-tooltip-default">Hover!</span>
    <tonic-tooltip for="tonic-tooltip-default">
      Hello World
    </tonic-tooltip>
  </div>

  <!-- Default tooltip w/ image -->
  <div class="test-container">
    <span>Default tooltip w/ image</span>
    <span id="tonic-tooltip-image">Hover!</span>
    <tonic-tooltip for="tonic-tooltip-image">
      <img src="/sampleprofile.jpg" width="100px">
    </tonic-tooltip>
  </div>

  <!-- Default tooltip width-->
  <div class="test-container">
    <span>width="400px"</span>
    <span id="tonic-tooltip-width">Hover!</span>
    <tonic-tooltip
      for="tonic-tooltip-width"
      width="400px">
      Hello World
    </tonic-tooltip>
  </div>

  <!-- Default tooltip height -->
  <div class="test-container">
    <span>height="250px"</span>
    <span id="tonic-tooltip-height">Hover!</span>
    <tonic-tooltip
      for="tonic-tooltip-height"
      height="250px">
      Hello World
    </tonic-tooltip>
  </div>

  <!-- Default tooltip theme light -->
  <div class="test-container">
    <span>theme="light"</span>
    <span id="tonic-tooltip-theme-light">Hover!</span>
    <tonic-tooltip
      for="tonic-tooltip-theme-light"
      theme="light">
      I am a light theme!
    </tonic-tooltip>
  </div>

  <!-- Default tooltip theme dark -->
  <div class="test-container">
    <span>theme="dark"</span>
    <span id="tonic-tooltip-theme-dark">Hover!</span>
    <tonic-tooltip
      for="tonic-tooltip-theme-dark"
      theme="dark">
      I am a dark theme!
    </tonic-tooltip>
  </div>

</section>
`)

// TODO write tests
