const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
const Tonic = require('@optoolco/tonic')
components(Tonic)

class InputWrapper extends Tonic {
  constructor (o) {
    super(o)

    this.state = {
      copyValue: '',
      currentValue: '',
      inputEvents: 0,
      ...this.state
    }
  }

  input () {
    this.state.inputEvents++

    const input = this.querySelector('tonic-input')
    this.state.currentValue = input.value
  }

  click (ev) {
    const el = Tonic.match(ev.target, '[data-event]')
    if (!el || el.dataset.event !== 'copy') return

    this.state.copyValue = this.state.currentValue
    this.reRender()
  }

  render () {
    return this.html`
      <div>
        <tonic-input id='tonic-input-wrapped'>
        </tonic-input>
        <tonic-button data-event='copy'>Copy</tonic-button>
        <span class='display'>${this.state.copyValue}</span>
      </div>
    `
  }
}

Tonic.add(InputWrapper, 'input-test-wrapper-comp')

document.body.appendChild(html`
<section id="input">
  <h2>Input</h2>

  <div class="test-container">
    <span>Default Input</span>
    <tonic-input id="input-1">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>value="I'm a value"</span>
    <tonic-input
      id="input-2"
      value="I'm a value">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>type="email"</span>
    <tonic-input
      id="input-3"
      type="email">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>required="true"</span>
    <tonic-input
      id="input-4"
      required="true">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>disabled="true"</span>
    <tonic-input
     id="input-5"
     disabled="true">
   </tonic-input>
  </div>

  <div class="test-container">
    <span>spellcheck="false"</span>
    <tonic-input
      id="input-6"
      spellcheck="false">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>error-message="nope"</span>
    <tonic-input
      id="input-7"
      value="test"
      type="email"
      error-message="nope">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>placeholder="test"</span>
    <tonic-input
      id="input-8"
      placeholder="test">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>label="Label"</span>
    <tonic-input
      id="input-9"
      label="Label">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>tabindex="0"</span>
    <tonic-input
      id="input-10"
      tabindex="0">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>readonly="true"</span>
    <tonic-input
      id="input-11"
      readonly="true">
    </tonic-input>
  </div>

  <div class="test-container">
    <span>Test Wrapper Comp</span>
    <input-test-wrapper-comp id="wrap"></input-test-wrapper-comp>
  </div>

  <!-- <div class="test-container">
    <span>src="somePath"</span>
    <tonic-input src="./sprite.svg#custom_off"></tonic-input>
  </div> -->

  <!-- <div class="test-container">
    <span>position="left"</span>
    <tonic-input position="left" src="./sprite.svg#custom_off"></tonic-input>
  </div> -->

  <!-- <div class="test-container">
    <span>position="right"</span>
    <tonic-input position="right" src="./sprite.svg#custom_off"></tonic-input>
  </div> -->

  <!-- <div class="test-container">
    <span>pattern="[A-Za-z\s]+"</span>
    <tonic-input pattern="[A-Za-z\s]+"></tonic-input>
  </div> -->


  <!-- <div id="input-11" class="test-container">
    <span>minlength="5"</span>
    <tonic-input minlength="5"></tonic-input>
  </div>

  <div class="test-container">
    <span>maxlength="10"</span>
    <tonic-input maxlength="10"></tonic-input>
  </div>

  <div class="test-container">
    <span>type="number", min="5"</span>
    <tonic-input type="number" min="5"></tonic-input>
  </div>

  <div class="test-container">
    <span>type="number", max="10"</span>
    <tonic-input type="number" max="10"></tonic-input>
  </div>

  <div class="test-container">
    <span>width="60px"</span>
    <tonic-input width="60px"></tonic-input>
  </div>

  <div class="test-container">
    <span>width="100%"</span>
    <tonic-input width="100%"></tonic-input>
  </div>

  <div class="test-container">
    <span>radius="8px"</span>
    <tonic-input radius="8px"></tonic-input>
  </div> -->

</section>
`)

tape('{{input-1}} default state is constructed', t => {
  const component = qs('tonic-input#input-1')
  const wrapper = qs('.tonic--wrapper', component)
  const input = qs('input', wrapper)
  const invalid = qs('.tonic--invalid', wrapper)

  t.plan(4)

  t.ok(wrapper, 'the component was constructed, has a wrapper')
  t.ok(input, 'the component contains an input')
  t.ok(invalid, 'the component contains a tonic invalid div')
  t.ok(!input.hasAttribute('disabled'), 'the input is not disabled by default')

  t.end()
})

tape('{{input-2}} contains a default value', t => {
  const component = qs('tonic-input#input-2')
  const input = qs('input', component)
  const value = component.getAttribute('value')

  t.plan(2)

  t.equal(value, input.getAttribute('value'), 'component input value attributes match')
  t.equal(value, input.value, 'component value attribute matches input value')

  t.end()
})

tape('{{input-3}} contains a type', t => {
  const component = qs('tonic-input#input-3')
  const input = qs('input', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.ok(component.hasAttribute('type'), 'component input value attributes match')
  t.equal(component.getAttribute('type'), input.type, 'component type matches input type')

  t.end()
})

tape('{{input-4}} is required', t => {
  const component = qs('tonic-input#input-4')
  const input = qs('input', component)

  t.plan(4)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.getAttribute('required'), 'true', 'component contains required attribute')
  t.equal(input.required, true, 'input is required')

  const styles = window.getComputedStyle(input)
  t.equal(styles.borderColor, 'rgb(204, 204, 204)')

  t.end()
})

tape('{{input-5}} is disabled', t => {
  const component = qs('tonic-input#input-5')
  const input = qs('input', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.getAttribute('disabled'), 'true', 'component contains disabled="true" attribute')
  t.ok(input.hasAttribute('disabled'), 'input has disabled attribute')

  t.end()
})

tape('{{input-6}} has spellcheck attribute', t => {
  const component = qs('tonic-input#input-6')
  const input = qs('input', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.ok(component.hasAttribute('spellcheck'), 'component contains spellcheck attribute')
  t.equal(component.getAttribute('spellcheck'), input.getAttribute('spellcheck'), 'component spellcheck attr matches input')

  t.end()
})

tape('{{input-7}} shows error message', t => {
  const component = qs('tonic-input#input-7')
  const input = qs('input', component)
  const error = qs('.tonic--invalid', component)
  const errorMessage = qs('span', error)

  t.plan(4)

  t.ok(input, 'the component was constructed with an input')
  t.ok(error, 'the component was constructed with an error')
  t.equal(component.getAttribute('error-message'), errorMessage.textContent, 'attribute matches error text')
  t.ok(qs('input:invalid', component), 'input is invalid, error is showing')

  t.end()
})

tape('{{input-8}} has placeholder', t => {
  const component = qs('tonic-input#input-8')
  const input = qs('input', component)

  t.plan(2)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.getAttribute('placeholder'), input.getAttribute('placeholder'), 'component and input placeholder attributes match')

  t.end()
})

tape('{{input-9}} has label', t => {
  const component = qs('tonic-input#input-9')
  const input = qs('input', component)
  const label = qs('label:not(.tonic--icon)', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.ok(label, 'the component was constructed with a label')
  t.equal(component.getAttribute('label'), label.textContent, 'component label attribute matches text of label')

  t.end()
})

tape('{{input-10}} has tabindex', t => {
  const component = qs('tonic-input#input-10')
  const input = qs('input', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.hasAttribute('tabindex'), false, 'component does not have a tabindex')
  t.equal(input.hasAttribute('tabindex'), true, 'input has a tabindex')

  t.end()
})

tape('{{input-11}} has readonly attribute', t => {
  const component = qs('tonic-input#input-11')
  const input = qs('input', component)

  t.plan(3)

  t.ok(input, 'the component was constructed with an input')
  t.equal(component.hasAttribute('readonly'), true, 'component has a readonly="true" attribute')
  t.ok(input.hasAttribute('readonly'), 'input has a readonly attribute')

  t.end()
})

tape('{{input-1}} event handlers', t => {
  const component = qs('tonic-input#input-1')
  const input = qs('input', component)

  let counter = 0

  component.addEventListener('input', (ev) => {
    t.equal(++counter, 1)
    t.equal(ev.currentTarget.value, 'someText')

    t.end()
  })

  input.value = 'someText'
  input.dispatchEvent(new window.Event('input', {
    bubbles: true
  }))
})

tape('input wrapper component interactions', t => {
  const comp = qs('input-test-wrapper-comp')

  let display = comp.querySelector('span.display')
  t.equal(display.textContent, '')

  const rawInputs = comp.querySelectorAll('input')
  t.equal(rawInputs.length, 1)

  const input = rawInputs[0]
  input.value = 'someText'
  input.dispatchEvent(new window.Event('input', {
    bubbles: true
  }))

  const buttons = comp.querySelectorAll('button')
  t.equal(buttons.length, 1)

  buttons[0].click()

  setTimeout(() => {
    display = comp.querySelector('span.display')
    t.equal(display.textContent, 'someText')
    t.equal(comp.state.inputEvents, 1)

    const input2 = comp.querySelector('input')
    t.equal(input2.value, 'someText')

    t.end()
  }, 20)
})
