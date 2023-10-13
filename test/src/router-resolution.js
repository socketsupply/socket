import test from 'socket:test'
import URL from 'socket:url'

const dirname = URL.resolve(import.meta.url, './router-resolution')

test('router-resolution', async (t) => {
  const response = await fetch(dirname + '/invalid')
  t.equal(response.status, 404, '404 on invalid route')
})
