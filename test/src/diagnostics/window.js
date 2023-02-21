import diagnostics from 'socket:diagnostics'
import test from 'socket:test'

test('diagnostics - window - metrics', async (t) => {
  diagnostics.window.metrics.init()

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
        t.equal(typeof message.fps, 'number', 'message.fps is number')
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
    const uri = 'ipc://platform.primordials'
    const options = { method: 'GET' }
    let diagnosticsReceived = false
    diagnostics.window.metrics.subscribe('fetch', (message) => {
      t.equal(message.resource, uri, 'message.resource === uri')
      t.equal(message.options, options, 'message.options === options')
      diagnosticsReceived = true
    })

    fetch(uri, options)
      .then((response) => response.json())
      .then((json) => t.ok(json?.data, 'response json.data'))
      .then(() => t.ok(diagnosticsReceived, 'fetch diagnostics received'))
      .then(resolve)
  })

  await new Promise((resolve) => {
    const pending = []
    const uri = 'ipc://platform.primordials'
    const method = 'GET'

    pending.push(new Promise((resolve) => {
      diagnostics.window.metrics.subscribe('XMLHttpRequest.open', (message) => {
        resolve()
      })
    }))

    pending.push(new Promise((resolve) => {
      diagnostics.window.metrics.subscribe('XMLHttpRequest.send', (message) => {
        resolve()
      })
    }))

    const request = new globalThis.XMLHttpRequest()
    request.open(method, uri, true)
    request.send()

    Promise.all(pending).then(resolve)
  })

  diagnostics.window.metrics.destroy()
})
