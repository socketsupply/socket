const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))
const { TonicForm } = require('.')

document.body.appendChild(html`
<section id="form">
  <h2>Form</h2>

  <div class="test-container">
    <span>Test Form</span>
    <tonic-form id="f1">
      <tonic-input data-key="ka" id="a" value="va">
      </tonic-input>

      <tonic-input data-key="foo.kb" id="b" value="vb">
      </tonic-input>

      <tonic-input id="c" value="vc">
      </tonic-input>

      <tonic-input data-key="bar.0.buzz" id="d" value="vd">
      </tonic-input>
    </tonic-form>
  </div>

  <div class="test-container">
    <span>Test Form</span>
    <tonic-form id="f2">
      <tonic-input data-key="ka" id="a2">
      </tonic-input>

      <tonic-input data-key="foo.kb" id="b2">
      </tonic-input>

      <tonic-input id="c2">
      </tonic-input>

      <tonic-input data-key="bar.0.buzz" id="d2">
      </tonic-input>
    </tonic-form>
  </div>

</section>
`)

tape('{{form-1}} get data from form', t => {
  const component = qs('tonic-form#f1')
  const inputA = qs('#a', component)
  const inputB = qs('#b', component)
  const inputD = qs('#d', component)

  t.plan(2)

  t.ok(inputA && inputB && inputD, 'the component contains the correct number of inputs')

  const expected = {
    ka: 'va',
    foo: {
      kb: 'vb'
    },
    bar: [{ buzz: 'vd' }]
  }

  t.deepEqual(expected, component.value)

  t.end()
})

tape('{{form-2}} populate form data', t => {
  const component = qs('tonic-form#f2')
  const inputA = qs('#a2', component)
  const inputB = qs('#b2', component)
  const inputD = qs('#d2', component)

  const data = {
    ka: 'va',
    foo: {
      kb: 'vb'
    },
    bar: [{ buzz: 'vd' }]
  }

  component.value = data

  t.equal(inputA.value, 'va')
  t.equal(inputB.value, 'vb')
  t.equal(inputD.value, 'vd')

  t.end()
})

tape('{{form-3}} get and set data', t => {
  var o = {
    a: {
      bbb: {
        c: [
          'aa',
          { a: 'bb' },
          'cc'
        ]
      }
    }
  }

  const path = 'a.bbb.c.1.a'

  t.equal(TonicForm.getPropertyValue(o, path), 'bb')
  t.ok(TonicForm.setPropertyValue(o, path, 'xx'))
  t.equal(TonicForm.getPropertyValue(o, path), 'xx')

  t.equal(TonicForm.getPropertyValue(o, 'a.bbb.c.4.a'), null)

  t.ok(!TonicForm.getPropertyValue({ a: {} }, path))
  t.ok(!TonicForm.getPropertyValue(null, path))
  t.ok(!TonicForm.getPropertyValue(undefined, path))
  t.ok(!TonicForm.getPropertyValue(undefined, undefined))

  const oo = { a: 0 }
  t.ok(!TonicForm.setPropertyValue(undefined, undefined))
  t.ok(TonicForm.getPropertyValue(oo, 'a') === 0)
  t.ok(TonicForm.setPropertyValue(oo, 'a', 1))
  t.ok(TonicForm.getPropertyValue(oo, 'a') === 1)

  const ooo = {}
  t.ok(TonicForm.setPropertyValue(ooo, 'a.b.2.c', 'x'))
  t.ok(TonicForm.getPropertyValue(ooo, 'a.b.2.c') === 'x')

  const actual = JSON.stringify(ooo)
  const expected = '{"a":{"b":[null,null,{"c":"x"}]}}'
  t.ok(expected === actual)
  t.end()
})
