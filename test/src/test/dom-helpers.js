import { test } from 'socket:test'

test('socket:test sleep and requestAnimationFrame', async (t) => {
  await t.sleep(100)
  await t.requestAnimationFrame()
  await t.requestAnimationFrame('Raf with a comment')
})

test('socket:test wait for', async (t) => {
  const buttonEl = document.createElement('button')
  buttonEl.innerText = 'Push Me'
  buttonEl.id = 'push-me-button'

  setTimeout(async () => {
    await t.appendChild(document.body, buttonEl, 'Delay adding button to body')
  }, 500)

  await t.waitFor('#push-me-button')
  await t.waitForText(document.body, 'Push Me')

  await t.removeElement('#push-me-button')

  const wrapper = document.createElement('div')
  wrapper.id = 'wrapper'
  t.appendChild(document.body, wrapper)

  wrapper.innerHTML = `
    <span>foo</span><span>bar</span><span>baz</span>
  `
  await t.waitForText(wrapper, {
    text: 'baz',
    multipleTags: true
  })

  await t.removeElement(wrapper)

  const testEl = document.createElement('div')
  testEl.setAttribute('id', 'test-el')
  await t.appendChild(document.body, testEl)
  const children = Array.prototype.map.call('quux', letter => {
    const el = document.createElement('span')
    el.textContent = letter
    return el
  })
  testEl.append(...children)

  try {
    const quux = await t.waitForText(testEl, {
      text: 'quux',
      multipleTags: true,
      timeout: 1000
    })

    t.ok(quux, 'should find the test in separate tags')
  } catch (err) {
    t.fail(err.toString())
  }

  try {
    const el = await t.waitForText(document.body, {
      text: 'uux',
      multipleTags: true,
      timeout: 1000
    })
    t.ok(t.elementVisible(el), 'should find the element')
  } catch (err) {
    t.fail(err.toString())
  }

  try {
    const match = await t.waitForText(document.body, {
      text: 'uux',
      multipleTags: true,
      timeout: 1000
    })

    t.equal(match.getAttribute('id'), 'test-el',
      'should return the parent element')
  } catch (err) {
    t.fail(err.toString())
  }

  await t.removeElement(testEl)
})

test('socket:test events and clicking', async (t) => {
  const buttonEl = document.createElement('button')
  buttonEl.innerText = 'Push Me'
  buttonEl.id = 'push-me-button'

  await t.appendChild(document.body, buttonEl, 'Delay adding button to body')

  buttonEl.addEventListener('click', () => {
    t.pass('The button was clicked')
  })

  await t.click('#push-me-button')
  await t.eventClick('#push-me-button')

  await t.dispatchEvent('click', '#push-me-button')

  await t.removeElement('#push-me-button')
})

test('socket:test events and typing focus and blur', async (t) => {
  const inputEl = document.createElement('input')
  inputEl.type = 'text'
  inputEl.id = 'my-text-input'

  await t.appendChild(document.body, inputEl)

  t.equal(inputEl.value, '', 'Input element value is just an empty string')
  t.ok(document.activeElement !== inputEl, 'input element isnt focused')
  await t.focus('#my-text-input')
  t.ok(document.activeElement === inputEl, 'input is focused')
  await t.type('#my-text-input', 'hello world')

  t.equal(inputEl.value, 'hello world', 'Input element value was changed by typing')
  await t.blur('#my-text-input')
  t.ok(document.activeElement !== inputEl, 'input element isnt focused')

  await t.removeElement('#my-text-input')
})

test('socket:test visibility', async (t) => {
  const wrapper = document.createElement('div')
  wrapper.id = 'visibility-tests'

  wrapper.innerHTML = `
    <div id="inner-element">Hello World</div>
  `

  await t.appendChild(document.body, wrapper)
  await t.elementVisible('#inner-element')
  const innerEl = t.querySelector('#inner-element')
  innerEl.hidden = true
  await t.elementInvisible('#inner-element')
  await t.removeElement(wrapper)
})

test('socket:test events and typing focus and blur', async (t) => {
  const wrapper = document.createElement('div')
  wrapper.id = 'test-container'

  wrapper.innerHTML = `
    <div class='inner'>Hi</div>
    <div class='inner'>Hi</div>
    <div class='inner'>Hi</div>
    <div class='inner'>Hi</div>
    <div class='inner'>Hi</div>
  `
  await t.appendChild(document.body, wrapper)

  t.querySelector('.inner')

  const results = t.querySelectorAll('.inner')
  t.equal(results.length, 5, '5 elements selected')

  await t.removeElement(wrapper)
})
