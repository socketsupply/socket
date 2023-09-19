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
    'window.js',
    'application.js',
    'bluetooth.js',
    // 'bootstrap.js', // don't document this module yet
    'crypto.js',
    'dgram.js',
    'dns/index.js',
    'dns/promises.js',
    'events.js',
    'fs/index.js',
    'fs/promises.js',
    'ipc.js',
    'os.js',
    'network.js',
    'path/path.js',
    'process.js',
    'window.js'
  ]

  const dest = JS_INTERFACE_DIR
  const md = 'README.md'

  const chunks = await Promise.all(modules.map(async module => ({
    module,
    content: generateApiModuleDoc({
      src: await fs.readFile(path.relative(process.cwd(), `${dest}/${module}`)),
      srcFile: path.relative(process.cwd(), `${dest}/${module}`),
      gitTagOrBranch
    })
  })))

  // modules special with special handling
  chunks.push({
    module: 'buffer.js',
    content: `
# [Buffer](https://github.com/socketsupply/socket/blob/master/api/buffer.js)

Buffer module is a [third party](https://github.com/feross/buffer) vendor module provided by Feross Aboukhadijeh and other contributors (MIT License),

External docs: https://nodejs.org/api/buffer.html
`
  })

  const result = chunks
    // sort by module name
    .sort((a, b) => a.module > b.module ? 1 : -1)
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

  const result = generateApiModuleDoc({ src, srcFile })

  const destFile = path.relative(process.cwd(), `${dest}/${md}`)
  fs.writeFile(destFile, result)
}

// socket/api/CONFIG.md
{
  const src = path.relative(process.cwd(), 'src/cli/templates.hh')
  const source = await fs.readFile(src, 'utf8')

  const config = generateConfig(source)
  fs.writeFile('api/CONFIG.md', config)
}
