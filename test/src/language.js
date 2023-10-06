import language from 'socket:language'
import test from 'socket:test'

test('language.lookup(query[, options])', (t) => {
  for (const name of language.names) {
    const results = language.lookup(name, { strict: true })
    t.ok(results.length === 1, 'exactly 1 result in strict lookup by name: ' + name)
    t.ok(results[0]?.name === name, 'result.name === name')
    t.ok(results[0]?.code, 'result.code')
    t.ok(results[0]?.tags.length, 'results[0].tags')
  }

  for (const code of language.codes) {
    const results = language.lookup(code, { strict: true })
    t.ok(results.length === 1, 'exactly 1 result in strict lookup by code: ' + code)
    t.ok(results[0].code === code, 'result.code === code')
    t.ok(results[0].name, 'result.name')
    t.ok(results[0].tags.length, 'results[0].tags')
  }
})

test('language.describe(tag[, options])', (t) => {
  for (const tag of language.tags) {
    const results = language.describe(tag, { strict: true })
    if (results.length) {
      t.ok(results.length, 'results.length > 0')
      t.ok(results[0]?.tag === tag, 'result.tag === ' + tag)
      t.ok(results[0]?.code, 'result.code')
      t.ok(results[0]?.description, 'result.description')
    }
  }
})
