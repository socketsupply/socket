const test = require('socket:test')

test('commonjs - globals', (t) => {
  t.ok(process === require('socket:process'), 'process is global')
  t.ok(typeof global === 'object' && global.console && global.process, 'global is global')
  t.ok(module, 'module is global')
  t.ok(exports, 'exports is global')
  t.ok(__filename, '__filename is global')
  t.ok(__dirname, '__filename is global')
})
