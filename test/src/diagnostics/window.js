import diagnostics from 'socket:diagnostics'
import test from 'socket:test'

test('diagnostics - window - metrics', async (t) => {
  diagnostics.window.metrics.start()

  /**
   * This test requires the browser window to be focused as
   * requestAnimationFrame` is used internally.
   * Uncomment it to verify manually
   */
  /*
  await new Promise((resolve) => {
    let iterations = 0
    diagnostics.window.metrics.subscribe('requestAnimationFrame', (message) => {
      if (message.rate > 0 && iterations === 30) {
        t.equal(typeof message.rate, 'number', 'message.rate is number')
        t.equal(typeof message.samples, 'number', 'message.samples is number')
      }

      resolve()
    })

    globalThis.requestAnimationFrame(function onAnimationFrame () {
      if (++iterations < 30) {
        globalThis.requestAnimationFrame(onAnimationFrame)
      }
    })
  })
  */

  await new Promise((resolve) => {
    const options = { method: 'GET' }
    const uri = window.location.href
    let diagnosticsReceived = false

    diagnostics.window.metrics.subscribe('fetch', (message) => {
      t.equal(message.resource, uri, 'message.resource === uri')
      t.equal(message.options, options, 'message.options === options')
      diagnosticsReceived = true
    })

    fetch(uri, options)
      .then((response) => response.text())
      .then((text) => t.ok(typeof text === 'string', 'response.text()'))
      .then(() => t.ok(diagnosticsReceived, 'fetch diagnostics received'))
      .catch((err) => t.ifError(err))
      .then(resolve)
  })

  await new Promise((resolve) => {
    const pending = []
    const method = 'GET'
    const uri = window.location.href

    pending.push(new Promise((resolve) => {
      diagnostics.window.metrics.subscribe('XMLHttpRequest.open', (message) => {
        t.ok(message, 'window.XMLHttpRequest.open')
        resolve()
      })
    }))

    pending.push(new Promise((resolve) => {
      diagnostics.window.metrics.subscribe('XMLHttpRequest.send', (message) => {
        t.ok(message, 'window.XMLHttpRequest.send')
        resolve()
      })
    }))

    const request = new globalThis.XMLHttpRequest()
    request.open(method, uri, true)
    request.send()

    Promise.all(pending).then(resolve)
  })

  diagnostics.window.metrics.stop()
})
