import application from 'socket:application'
import test from 'socket:test'

test('trigger from openExternal', async (t) => {
  const window = await application.getCurrentWindow()
  const expected = 'socket-tests://from-open-external?key=value'
  const pending = new Promise((resolve) => {
    globalThis.addEventListener('applicationurl', callback, { once: true })
    function callback (event) {
      t.equal(typeof event.source, 'string', 'event.source is string')
      t.equal(event.isValid, true, 'event.isValid is true')
      t.ok(event.url instanceof URL, 'event.url is URL')
      t.equal(event.url.protocol, 'socket-tests:', 'event.url.protocol is "socket-test"')
      t.equal(event.url.hostname, 'from-open-external', 'event.url.hostname === "from-open-external"')
      t.equal(event.url.searchParams.get('key'), 'value', 'event.url.searchParams.get("key") is "value"')
      resolve()
    }
  })

  try {
    const result = await window.openExternal({ value: expected })
    t.equal(result.url, expected, 'result.url === expected')
  } catch (err) {
    return t.fail(err)
  }

  await pending
})
