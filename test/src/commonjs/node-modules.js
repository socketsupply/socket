const test = require('socket:test')
const a = require('a')

test('commonjs - node modules', (t) => {
  t.ok(typeof a === 'object', 'a object (from node_modules)')
  t.ok(typeof a.b === 'object', 'a.b object (child)')
  t.ok(typeof a.b.c === 'string', 'a.b.c string (from node_modules)')
})
