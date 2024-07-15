#!/usr/bin/env node
import { execSync } from 'node:child_process'
import fs from 'node:fs/promises'
import path from 'node:path'
import { generateApiModuleDoc } from './docs-generator/api-module.js'
import { generateConfig } from './docs-generator/config.js'
import { generateCli } from './docs-generator/cli.js'

const VERSION = `v${(await fs.readFile('./VERSION.txt', 'utf8')).trim()}`
const isCurrentTag = execSync('git describe --tags --always').toString().trim() === VERSION

let tagNotPresentOnRemote = false
if (!isCurrentTag) {
  try {
    tagNotPresentOnRemote = execSync(`git ls-remote --tags origin ${VERSION}`).toString().length === 0
  } catch (err) {
    console.warn(err.message)
  }
}

const gitTagOrBranch = (isCurrentTag || tagNotPresentOnRemote) ? VERSION : 'master'

const JS_INTERFACE_DIR = 'api'
const SOCKET_NODE_DIR = 'npm/packages/@socketsupply/socket-node'

// socket/api/README.md
{
  const modules = [
    'application.js',
    'bluetooth.js',
    // 'bootstrap.js', // don't document this module yet
    'crypto.js',
    'dgram.js',
    'dns/index.js',
    'dns/promises.js',
    'fs/index.js',
    'fs/promises.js',
    'ipc.js',
    // 'location.js',
    'network.js',
    'os.js',
    'path/path.js',
    'process.js',
    'test/index.js',
    // 'test/dom-helpers.js',
    // 'url/index.js',
    'window.js'
  ]

  const dest = JS_INTERFACE_DIR
  const md = 'README.md'

  const chunks = await Promise.all(modules.map(async module => {
    const location = `${dest}/${module}`
    const src = await fs.readFile(path.relative(process.cwd(), location))
    return generateApiModuleDoc({ src, location, gitTagOrBranch })
  }))

  // modules special with special handling
  chunks.push({
    header: 'Buffer',
    content: `
# [Buffer](https://github.com/socketsupply/socket/blob/master/api/buffer.js)

Buffer module is a [third party](https://github.com/feross/buffer) vendor module provided by Feross Aboukhadijeh and other contributors (MIT License).

External docs: https://nodejs.org/api/buffer.html
`
  }, {
    header: 'Events',
    content: `
# [Events](https://github.com/socketsupply/socket/blob/master/api/events.js)

Events module is a [third party](https://github.com/browserify/events/blob/main/events.js) module provided by Browserify and Node.js contributors (MIT License).

External docs: https://nodejs.org/api/events.html
`
  })

  const result = chunks
    // sort by header
    .sort((a, b) => a.header > b.header ? 1 : -1)
    // get content
    .map(chunk => chunk.content)
    // join
    .join('\n')

  const destFile = path.relative(process.cwd(), `${dest}/${md}`)
  fs.writeFile(destFile, result)
}

// socket-node/API.md
{
  const filename = 'index.js'
  const dest = SOCKET_NODE_DIR
  const location = `${dest}/${filename}`
  const md = 'API.md'

  const srcFile = path.relative(process.cwd(), location)
  const src = await fs.readFile(srcFile)

  const { content } = generateApiModuleDoc({ src, location })

  const destFile = path.relative(process.cwd(), `${dest}/${md}`)
  fs.writeFile(destFile, content)
}

const templateFilePath = path.relative(process.cwd(), 'src/cli/templates.hh')
const templateFileSource = await fs.readFile(templateFilePath, 'utf8')

// socket/api/CONFIG.md
{
  const config = generateConfig(templateFileSource)
  fs.writeFile('api/CONFIG.md', config)
}

// socket/api/CLI.md
{
  const cli = generateCli(templateFileSource)
  fs.writeFile('api/CLI.md', cli)
}
