const { test } = await import('socket:test')

test('commonjs - special top level await', (t) => {
  t.ok(true, 'works')
})
