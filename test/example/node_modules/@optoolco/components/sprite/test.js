const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="sprite">
  <tonic-sprite></tonic-sprite>
  <h2>Sprite</h2>

  <!-- Default inline toaster -->
  <div class="test-container">
    <span>Sprite</span>
    <svg id="svg-close">
      <use href="#close" xlink:href="#close"></use>
    </svg>
  </div>

</section>
`)

tape('test an icon', t => {
  const svg = qs('#svg-close')
  const use = qs('use', svg)

  t.ok(svg)
  t.ok(svg.querySelector('use'))

  const size = use.getBoundingClientRect()
  t.equal(Math.floor(size.height), 92)
  t.equal(Math.floor(size.width), 92)

  const sprite = qs('tonic-sprite')
  t.ok(sprite)

  const spriteSize = sprite
    .querySelector('svg')
    .getBoundingClientRect()
  t.equal(spriteSize.height, 0)

  t.end()
})
