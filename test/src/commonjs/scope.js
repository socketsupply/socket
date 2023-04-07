// const path = require('socket:path')
const test = require('socket:test')

test('commonjs - scope', (t) => {
  t.equal(module.exports, exports, 'module.exports === exports')
  console.log({ __filename, __dirname })
})
