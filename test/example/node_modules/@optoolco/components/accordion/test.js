const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="accordion">
  <h2>Accordion</h2>

  <div id="accordion-1" class="test-container">
    <span>Default</span>
    <tonic-accordion id="accordion-parent-1">
      <tonic-accordion-section
        name="accordion-test-1"
        id="accordion-test-1"
        label="Accordion Test 1">
        Whatever
      </tonic-accordion-section>
      <tonic-accordion-section
        name="accordion-test-2"
        id="accordion-test-2"
        label="Accordion Test 2">
        Some Content
      </tonic-accordion-section>
      <tonic-accordion-section
        name="accordion-test-3"
        id="accordion-test-3"
        label="Accordion Test 3">
        The visual design includes features intended to help users understand that the accordion provides enhanced keyboard navigation functions. When an accordion header button has keyboard focus, the styling of the accordion container and all its header buttons is changed.
      </tonic-accordion-section>
    </tonic-accordion>
  </div>

  <div id="accordion-2" class="test-container">
    <span>Multiple</span>
    <tonic-accordion multiple="true" id="accordion-parent-2">
      <tonic-accordion-section
        name="accordion-test-4"
        id="accordion-test-4"
        label="Accordion Test 4">
        Whatever
      </tonic-accordion-section>
      <tonic-accordion-section
        name="accordion-test-5"
        id="accordion-test-5"
        label="Accordion Test 5">
        Some Content
      </tonic-accordion-section>
      <tonic-accordion-section
        name="accordion-test-6"
        id="accordion-test-6"
        label="Accordion Test 6">
        The visual design includes features intended to help users understand that the accordion provides enhanced keyboard navigation functions. When an accordion header button has keyboard focus, the styling of the accordion container and all its header buttons is changed.
      </tonic-accordion-section>
    </tonic-accordion>
  </div>
</section>`)

tape('{{accordion-1}} has correct default state', t => {
  const container = qs('#accordion-1')
  const component = qs('tonic-accordion', container)

  t.ok(component, 'rendered')
  t.end()
})

tape('{{accordion-2}} is styled', t => {
  const container = qs('#accordion-2')
  const comp = qs('tonic-accordion', container)

  const styles = window.getComputedStyle(comp)
  t.equal(styles.borderWidth, '1px')

  const button = qs('tonic-accordion-section button', container)
  const styles2 = window.getComputedStyle(button)

  t.equal(styles2.fontSize, '14px')
  t.end()
})
