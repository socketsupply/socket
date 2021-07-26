const { execSync } = require('child_process')
const fs = require('fs')

const test = require('tape')

test('check for bin', t => {
  try {
    fs.statSync('./bin/cli')
  } catch (err) {
    t.fail(err)
  }

  t.end()
})

if (process.platform === 'darwin') {
  test('print help', t => {
    const r = execSync('./bin/opkit -h')
    console.log(r)
    t.end()
  })
}
