import language from 'socket:language'
import test from 'socket:test'

test('language.lookup(query[, options])', (t) => {
  const pass = {
    names: true,
    codes: true
  }

  for (const name of language.names) {
    const results = language.lookup(name, { strict: true })
    pass.names = pass.name && (
      results.length &&
      results[0]?.name &&
      results[0]?.code &&
      results[0]?.tags.length
    )
  }

  for (const code of language.codes) {
    const results = language.lookup(code, { strict: true })
    pass.codes = pass.codes && (
      results.length === 1 &&
      results[0].code === code &&
      results[0].name &&
      results[0].tags.length
    )
  }

  t.ok(pass.names, 'lookup by name')
  t.ok(pass.codes, 'lookup by code')
})

test('language.describe(tag[, options])', (t) => {
  let pass = true
  for (const tag of language.tags) {
    const results = language.describe(tag, { strict: true })
    if (results.length) {
      pass = pass && (
        results.length &&
        results[0]?.tag === tag &&
        results[0]?.code &&
        results[0]?.description
      )
    }
  }

  t.ok(pass, 'language descriptions')
})
