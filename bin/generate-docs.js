#!/usr/bin/env node
import { execSync } from 'node:child_process'
import fs from 'node:fs/promises'
import path from 'node:path'
import { generateApiModuleDoc } from './docs-generator/api-module.js'
import { generateConfig } from './docs-generator/config.js'

const VERSION = `v${(await fs.readFile('./VERSION.txt', 'utf8')).trim()}`
const isCurrentTag = execSync('git describe --tags --always').toString().trim() === VERSION
const tagNotPresentOnRemote = execSync(`git ls-remote --tags origin ${VERSION}`).toString().length === 0

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
    // 'os.js',
    'path/path.js',
    'process.js',
    // 'test/index.js',
    // 'url/index.js',
    'window.js'
  ]

  const dest = JS_INTERFACE_DIR
  const md = 'README.md'

  const chunks = await Promise.all(modules.map(async module => {
    const src = await fs.readFile(path.relative(process.cwd(), `${dest}/${module}`))
    const srcFile = path.relative(process.cwd(), `${dest}/${module}`)
    const { content, header } = generateApiModuleDoc({ src, srcFile, gitTagOrBranch })
    return { module, content, header }
  }))

  // modules special with special handling
  chunks.push({
    module: 'buffer.js',
    header: 'Buffer',
    content: `
# [Buffer](https://github.com/socketsupply/socket/blob/master/api/buffer.js)

Buffer module is a [third party](https://github.com/feross/buffer) vendor module provided by Feross Aboukhadijeh and other contributors (MIT License).

External docs: https://nodejs.org/api/buffer.html
`
  }, {
    module: 'events.js',
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
  const md = 'API.md'

  const srcFile = path.relative(process.cwd(), `${dest}/${filename}`)
  const src = await fs.readFile(srcFile)

  const { content } = generateApiModuleDoc({ src, srcFile })

  const destFile = path.relative(process.cwd(), `${dest}/${md}`)
  fs.writeFile(destFile, content)
}

// socket/api/CONFIG.md
{
  const src = path.relative(process.cwd(), 'src/cli/templates.hh')
  const source = await fs.readFile(src, 'utf8')

  const config = generateConfig(source)
  fs.writeFile('api/CONFIG.md', config)
}
