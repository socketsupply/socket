const Tonic = require('@optoolco/tonic')
const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="tabs">
  <h2>Tabs</h2>

  <!-- Default tabs -->
  <div id="tabs-1" class="test-container">
    <span>Default Tabs</span>

    <tonic-tabs id="tonic-tabs-1" selected="tab-2">
      <tonic-tab
        id="tab-1"
        for="tab-panel-1">One</tonic-tab>
      <tonic-tab
        id="tab-2"
        for="tab-panel-2">Two</tonic-tab>
      <tonic-tab
        id="tab-3"
        for="tab-panel-3">Three</tonic-tab>
    </tonic-tabs>

    <tonic-tab-panel id="tab-panel-1">Content One</tonic-tab-panel>
    <tonic-tab-panel id="tab-panel-2">Content Two</tonic-tab-panel>
    <tonic-tab-panel id="tab-panel-3">Content Three</tonic-tab-panel>
  </div>

  <!-- Default tabs -->
  <div id="tabs-2" class="test-container">
    <span>Tabs with nesting</span>

    <tonic-tabs id="tonic-tabs-2" selected="tab-4">
      <tonic-tab
        id="tab-4"
        for="tab-panel-4">One</tonic-tab>
      <tonic-tab
        id="tab-5"
        for="tab-panel-5">Two</tonic-tab>
      <tonic-tab
        id="tab-6"
        for="tab-panel-6">Three</tonic-tab>
    </tonic-tabs>

    <tonic-tab-panel id="tab-panel-4">
      <tonic-select value="b">
        <option value="a">a</option>
        <option value="b">b</option>
        <option value="c">c</option>
      </tonic-select>
    </tonic-tab-panel>
    <tonic-tab-panel id="tab-panel-5">

      <tonic-accordion id="accordion-1">
        <tonic-accordion-section
          name="accordion-test-4b"
          id="accordion-test-4b"
          data="preview"
          label="Accordion Test 4b">
          Whatever
        </tonic-accordion-section>
        <tonic-accordion-section
          name="accordion-test-5b"
          id="accordion-test-5b"
          label="Accordion Test 5b">
          Some Content
        </tonic-accordion-section>
        <tonic-accordion-section
          name="accordion-test-6b"
          id="accordion-test-6b"
          label="Accordion Test 6b">
          The visual design includes features intended to help users understand that the accordion provides enhanced keyboard navigation functions. When an accordion header button has keyboard focus, the styling of the accordion container and all its header buttons is changed.
        </tonic-accordion-section>
      </tonic-accordion>
    </tonic-tab-panel>
    <tonic-tab-panel id="tab-panel-6">Content Three</tonic-tab-panel>
  </div>

  <!-- Tabs inside another component -->
  <div id="tabs-3" class="test-container">
    <component-container id="xxx">
      <span>Tabs inanother component</span>

      <tonic-tabs id="tabs-7" selected="tab-7">
        <tonic-tab
          id="tab-7"
          for="tab-panel-7">One</tonic-tab>
        <tonic-tab
          id="tab-8"
          for="tab-panel-8">Two</tonic-tab>
        <tonic-tab
          id="tab-9"
          for="tab-panel-9">Three</tonic-tab>
      </tonic-tabs>

      <tonic-tab-panel id="tab-panel-7">
        <tonic-select value="b">
          <option value="a">a</option>
          <option value="b">b</option>
          <option value="c">c</option>
        </tonic-select>
      </tonic-tab-panel>

      <tonic-tab-panel id="tab-panel-8">

        <tonic-accordion id="accordion-2">
          <tonic-accordion-section
            name="accordion-test-7"
            id="accordion-test-7"
            data="preview"
            label="Accordion Test 7">
            Whatever
          </tonic-accordion-section>
          <tonic-accordion-section
            name="accordion-test-8"
            id="accordion-test-8"
            label="Accordion Test 8">
            Some Content
          </tonic-accordion-section>
          <tonic-accordion-section
            name="accordion-test-9"
            id="accordion-test-9"
            label="Accordion Test 9">
            The visual design includes features intended to help users understand that the accordion provides enhanced keyboard navigation functions. When an accordion header button has keyboard focus, the styling of the accordion container and all its header buttons is changed.
          </tonic-accordion-section>
        </tonic-accordion>
      </tonic-tab-panel>
      <tonic-tab-panel id="tab-panel-9">Content Three</tonic-tab-panel>
    </component-container>
  </div>
</section>
`)

class ComponentContainer extends Tonic {
  click () {
    this.reRender()
  }

  render () {
    return this.html`
      ${this.childNodes}
    `
  }
}

Tonic.add(ComponentContainer)

class TestTabTextBox extends Tonic {
  constructor (o) {
    super(o)

    TestTabTextBox.allocationCounter++
    this.renderCounter = 0
    this.willConnectCounter = 0
    this.connectedCounter = 0
    this.disconnectedCounter = 0
  }

  willConnect () {
    this.willConnectCounter++
  }

  connected () {
    this.connectedCounter++
  }

  disconnected () {
    this.disconnectedCounter++
    if (this.renderCounter > 0) {
      this.preventRenderOnReconnect = true
    }
  }

  render () {
    this.renderCounter++
    return this.html`<span>${this.props.text}</span>`
  }
}
Tonic.add(TestTabTextBox)

TestTabTextBox.allocationCounter = 0

tape('{{tabs-3}} has correct default state', t => {
  const container = qs('component-container')

  t.ok(container, 'rendered')
  t.end()
})

tape('tabs only render what is visible', async t => {
  document.body.appendChild(html`
    <div id="tabs-4" class="test-container">
      <tonic-tabs id="tc-tabs-4" selected="tc4-tab1" detatch-on-hide="true">
        <tonic-tab id="tc4-tab1" for="tc4-panel1">one</tonic-tab>
        <tonic-tab id="tc4-tab2" for="tc4-panel2">two</tonic-tab>
        <tonic-tab id="tc4-tab3" for="tc4-panel3">three</tonic-tab>
      </tonic-tabs>
      <main>
        <tonic-tab-panel id="tc4-panel1" detatch="true">
          <test-tab-text-box text="Text one"></text-box>
        </tonic-tab-panel>
        <tonic-tab-panel id="tc4-panel2" detatch="true">
          <test-tab-text-box text="Text two"></text-box>
        </tonic-tab-panel>
        <tonic-tab-panel id="tc4-panel3" detatch="true">
          <test-tab-text-box text="Text three"></text-box>
        </tonic-tab-panel>
      </main>
    </div>
  `)

  const container = document.getElementById('tabs-4')
  const $ = (query) => {
    return [...container.querySelectorAll(query)]
  }

  {
    const tabs = $('tonic-tab')
    const panels = $('tonic-tab-panel')

    t.equal(tabs.length, 3, 'expected 3 tabs')
    t.equal(panels.length, 1, 'expect 1 panel')
    t.equal(panels[0].id, 'tc4-panel1', 'correct panel shown')

    const textboxs = $('test-tab-text-box')

    t.equal(textboxs.length, 1, 'expect 1 textbox')
    t.equal(textboxs[0].textContent, 'Text one')
    t.equal(TestTabTextBox.allocationCounter, 3, 'expect 3 allocations')
    t.equal(textboxs[0].renderCounter, 1, 'expect 1 render')
    t.equal(textboxs[0].connectedCounter, 1, 'expect 1 connected')
    t.equal(textboxs[0].disconnectedCounter, 0, 'expect 1 disconnected')
  }

  const anchorTwo = $('#tc4-tab2 a')[0]
  anchorTwo.click()

  await sleep(3)

  {
    const panels = $('tonic-tab-panel')
    t.equal(panels.length, 1, 'expect 1 panel')
    t.equal(panels[0].id, 'tc4-panel2', 'correct panel shown')

    const textboxs = $('test-tab-text-box')

    t.equal(textboxs.length, 1, 'expect 1 textbox')
    t.equal(textboxs[0].textContent, 'Text two')
    t.equal(TestTabTextBox.allocationCounter, 3, 'expect 3 allocations')
    t.equal(textboxs[0].renderCounter, 1, 'expect 1 render')
    t.equal(textboxs[0].connectedCounter, 1, 'expect 1 connected')
    t.equal(textboxs[0].disconnectedCounter, 1, 'expected 1 disconnected')
  }

  const anchorThree = $('#tc4-tab3 a')[0]
  anchorThree.click()

  await sleep(3)

  {
    const panels = $('tonic-tab-panel')
    t.equal(panels.length, 1, 'expect 1 panel')
    t.equal(panels[0].id, 'tc4-panel3', 'correct panel shown')

    const textboxs = $('test-tab-text-box')

    t.equal(textboxs.length, 1, 'expect 1 textbox')
    t.equal(textboxs[0].textContent, 'Text three')
    t.equal(TestTabTextBox.allocationCounter, 3, 'expect 3 allocations')
    t.equal(textboxs[0].renderCounter, 1, 'expect 1 render')
    t.equal(textboxs[0].connectedCounter, 1, 'expect 1 connected')
    t.equal(textboxs[0].disconnectedCounter, 1, 'expected 1 disconnected')
  }

  const anchorOne = $('#tc4-tab1 a')[0]
  anchorOne.click()

  await sleep(3)

  {
    const panels = $('tonic-tab-panel')
    t.equal(panels.length, 1, 'expect 1 panel')
    t.equal(panels[0].id, 'tc4-panel1', 'correct panel shown')

    const textboxs = $('test-tab-text-box')

    t.equal(textboxs.length, 1, 'expect 1 textbox')
    t.equal(textboxs[0].textContent, 'Text one')
    t.equal(TestTabTextBox.allocationCounter, 3, 'expect 3 allocations')
    t.equal(textboxs[0].renderCounter, 1, 'expect 1 render')
    t.equal(textboxs[0].connectedCounter, 2, 'expect 2 connected')
    t.equal(textboxs[0].disconnectedCounter, 1, 'expected 1 disconnected')
  }

  anchorTwo.click()

  await sleep(3)

  {
    const panels = $('tonic-tab-panel')
    t.equal(panels.length, 1, 'expect 1 panel')
    t.equal(panels[0].id, 'tc4-panel2', 'correct panel shown')

    const textboxs = $('test-tab-text-box')

    t.equal(textboxs.length, 1, 'expect 1 textbox')
    t.equal(textboxs[0].textContent, 'Text two')
    t.equal(TestTabTextBox.allocationCounter, 3, 'expect 3 allocations')
    t.equal(textboxs[0].renderCounter, 1, 'expect 1 render')
    t.equal(textboxs[0].connectedCounter, 2, 'expect 2 connected')
    t.equal(textboxs[0].disconnectedCounter, 2, 'expected 1 disconnected')
  }

  t.end()
})

function sleep (n) {
  return new Promise((resolve) => {
    setTimeout(resolve, n)
  })
}
