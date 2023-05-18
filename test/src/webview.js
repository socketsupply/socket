import test from 'socket:test'

const loaded = new Promise((resolve) => {
  window.addEventListener('load', () => {
    resolve(true)
  }, { once: true })
})

test('window.onload', async (t) => {
  t.ok(await loaded, 'window.onload called')
})
