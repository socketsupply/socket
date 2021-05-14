'use strict'

/* global fixture, test, window */
import { Selector } from 'testcafe'

fixture`Test form`
  .page`http://localhost:8030/`

test('can click button', async (t) => {
  const form = Selector('.tc-test-form')

  const button = form.find('button').withText(/Submit/i)
  const input = form.find('#tonic--input_test-form-input')

  await t.typeText(input, 'sample text')
  await t.click(button)

  const span = form.find('span').withText(/Text is: sample text/i)
  await t.expect(span.visible).eql(true)

  const c = await t.eval(() => {
    return window.promiseCounter
  })
  await t.expect(c).eql(1)
})

test('can dispatch custome vent', async (t) => {
  const parent = Selector('custom-event-parent')

  {
    const span = parent.find('span').withText(/Text is: /i)
    await t.expect(span.visible).eql(true)
    await t.expect(span.textContent).eql('Text is: empty')
  }

  const clickMe = parent.find('.test-click-me')
  await t.click(clickMe)

  {
    const span = parent.find('span').withText(/Text is: /i)
    await t.expect(span.visible).eql(true)
    await t.expect(span.textContent).eql('Text is: hmm')
  }
})
