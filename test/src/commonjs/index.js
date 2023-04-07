const test = require('socket:test')

const alpha = require('./alpha')
const beta = require('./beta')
const json = require('./json')

test('commonjs - directories', (t) => {
  t.equal(alpha?.key, 'value', 'alpha.key == value')
  t.equal(beta?.directory?.key, 'value', 'beta.directory.key == value')
  t.equal(beta?.child?.key, 'value', 'beta.child.key == value')
})

test('commonjs - json', (t) => {
  t.equal(json?.key, 'value', 'json.key == value')
})
