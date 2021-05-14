const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="icon">
  <h2>Icon</h2>

  <div id="icon-1" class="test-container">
    <span>Default Icon</span>
    <tonic-icon
      tabindex="0"
      symbol-id="example"
      src="/sprite.svg">
    </tonic-icon>
  </div>

  <div id="icon-2" class="test-container">
    <span>size="40px"</span>
    <tonic-icon
      symbol-id="example"
      src="/sprite.svg"
      size="40px">
    </tonic-icon>
  </div>

  <div id="icon-3" class="test-container">
    <span>fill="cyan"</span>
    <tonic-icon
      symbol-id="example"
      src="/sprite.svg"
      fill="cyan">
    </tonic-icon>
  </div>

  <div id="icon-4" class="test-container">
    <span>symbol-id="custom_off"</span>
    <tonic-icon
      symbol-id="custom_off"
      src="/sprite.svg">
    </tonic-icon>
  </div>

  <div id="icon-5" class="test-container">
    <span>tabindex="0"</span>
    <tonic-icon
      tabindex="0"
      symbol-id="example"
      src="/sprite.svg">
    </tonic-icon>
  </div>

</section>
`)

tape('{{icon-1}} is constructed properly', t => {
  const container = qs('#icon-1')
  const component = qs('tonic-icon', container)

  t.plan(3)

  t.ok(component.firstElementChild, 'the component was constructed')
  t.ok(component.hasAttribute('src'), 'the component has a src')
  t.ok(component.hasAttribute('symbol-id'), 'the component has a symbol id')

  t.end()
})

tape('{{icon-2}} has size attribute', t => {
  const container = qs('#icon-2')
  const component = qs('tonic-icon', container)
  const svg = qs('svg', component)
  const use = qs('use', component)

  t.plan(6)

  t.ok(component.firstElementChild, 'the component was constructed')
  t.ok(component.hasAttribute('size'), 'the component has the size attribute')
  t.equal(component.getAttribute('size'), svg.style.width, 'the size attribute matches svg width')
  t.equal(component.getAttribute('size'), svg.style.height, 'the size attribute matches svg height')

  t.equal(use.getAttribute('width'), component.getAttribute('size'))
  t.equal(use.getAttribute('height'), component.getAttribute('size'))

  t.end()
})

tape('{{icon-3}} has color attribute', t => {
  const container = qs('#icon-3')
  const component = qs('tonic-icon', container)
  const use = qs('use', component)

  t.plan(3)

  t.ok(component.firstElementChild, 'the component was constructed')
  t.equal(component.getAttribute('fill'), use.getAttribute('fill'), 'the fill attribute on the component matches use')
  t.equal(use.getAttribute('fill'), use.getAttribute('color'), 'use has matching fill and color attributes')

  t.end()
})

tape('{{icon-4}} uses custom symbol', t => {
  const container = qs('#icon-4')
  const component = qs('tonic-icon', container)
  const svg = qs('svg', component)
  const id = component.getAttribute('symbol-id')
  const src = component.getAttribute('src')
  const use = qs('use', component)
  const url = `${src}#${id}`

  t.plan(5)

  t.ok(svg, 'the component was constructed with an svg')
  t.ok(id, 'the component has symbol id')
  t.ok(src, 'the component has src')
  t.equal(use.getAttribute('href'), url, 'the href attribute contains the correct url')
  t.equal(use.getAttribute('xlink:href'), url)

  t.end()
})

tape('{{icon-5}} has tabindex attribute', t => {
  const container = qs('#icon-5')
  const component = qs('tonic-icon', container)
  const id = component.getAttribute('symbol-id')
  const svg = qs('svg', component)

  t.plan(4)

  t.ok(svg, 'the component was constructed with an svg')
  t.ok(id, 'the component has symbol id')
  t.equal(component.hasAttribute('tabindex'), false, 'component does not have tabindex attribute')
  t.equal(svg.hasAttribute('tabindex'), true, 'svg has tabindex attribute')

  t.end()
})
