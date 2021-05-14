#!/usr/bin/env node
const { promises: fs } = require('fs')
const path = require('path')

async function main () {
  const root = path.join(__dirname, '..')
  const p = path.join(root, 'src', 'index.js')
  const s = await fs.readFile(p, 'utf8')

  const cjs = '\nif (typeof module === \'object\') module.exports = Tonic\n'

  fs.writeFile(path.join(root, 'index.js'), s + cjs, { flags: 'a' })

  const js = '\nexport default Tonic\n'

  fs.writeFile(path.join(root, 'index.esm.js'), s + js, { flags: 'a' })
}

main()
