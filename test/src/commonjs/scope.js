const test = require('socket:test')

test('commonjs - scope', (t) => {
  t.equal(module.exports, exports, 'module.exports === exports')
  t.equal(typeof __filename, 'string', '__filename is string')
  t.equal(typeof __dirname, 'string', '__dirname is string')
  t.ok(
    __filename.includes(__dirname) && __filename.indexOf(__dirname) === 0,
    '__dirname in __filename'
  )
})
