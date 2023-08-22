import test from 'socket:test'
import url from 'socket:url'

test('url.resolve(url, base)', (t) => {
  t.equal(
    url.resolve('/a/b/c', 'socket://com.app/index.html'),
    'socket://com.app/index.html',
    '/a/b/c ~ socket://com.app/index.html -> socket://com.app/index.html'
  )

  t.equal(
    url.resolve('socket:///a/b/c', '..'),
    'socket:///a/',
    'socket:///a/b/c ~ .. -> socket:///a/'
  )

  t.equal(url.resolve('/a/b/c', '..'), '/a/', '/a/b/c ~ .. -> /a/')
  t.equal(url.resolve('/a/b/c', '.'), '/a/b/', '/a/b/c ~ . -> /a/b/')
  t.equal(url.resolve('/a/b/c', '/'), '/', '/a/b/c ~ / -> /')
})
