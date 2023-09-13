#!/usr/bin/env node
import * as acorn from 'acorn'
import * as walk from 'acorn-walk'
import { execSync } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'

const VERSION = `v${fs.readFileSync('./VERSION.txt', 'utf8').trim()}`
const isCurrentTag = execSync('git describe --tags --always').toString().trim() === VERSION
const tagNotPresentOnRemote = execSync(`git ls-remote --tags origin ${VERSION}`).toString().length === 0

const gitTagOrBranch = (isCurrentTag || tagNotPresentOnRemote) ? VERSION : 'master'

const JS_INTERFACE_DIR = 'api'
const SOCKET_NODE_DIR = 'npm/packages/@socketsupply/socket-node'

try {
  fs.unlinkSync(`${JS_INTERFACE_DIR}/README.md`)
  fs.unlinkSync(`${SOCKET_NODE_DIR}/API.md`)
} catch {}

export function transform (filename, dest, md, gitTagOrBranch = 'master') {
  const srcFile = path.relative(process.cwd(), `${dest}/${filename}`)
  const destFile = path.relative(process.cwd(), `${dest}/${md}`)

  if (typeof filename === 'string') {
    let accumulateComments = []
    const comments = {}
    const src = fs.readFileSync(srcFile)
    const ast = acorn.parse(String(src), {
      tokens: true,
      comment: true,
      ecmaVersion: 'latest',
      sourceType: 'module',
      onToken: (token) => {
        comments[token.start] = accumulateComments
        accumulateComments = []
      },
      onComment: (block, comment) => {
        if (!block) return
        if (comment[0] !== '*') return // not a JSDoc comment

        comment = comment.replace(/^\s*\*/gm, '').trim()
        comment = comment.replace(/^\n/, '')
        accumulateComments.push(comment.trim())
      },
      locations: true
    })

    for (const [key, value] of Object.entries(comments)) {
      if (!value.length) delete comments[key] // release empty items
    }

    const docs = []

    const onNode = node => {
      const item = {
        sort: node.loc.start.line,
        location: `/${srcFile}#L${node.loc.start.line}`,
        type: node.type,
        name: node.name,
        export: node?.type.includes('Export'),
        header: comments[node.start]
      }

      if (item.header?.join('').includes('@ignore')) {
        return
      }

      if (item.header?.join('').includes('@module')) {
        item.type = 'Module'
        const name = item.header.join('').match(/@module\s*(.*)/)
        if (name) item.name = name[1]
      }

      if (item.header?.join('').includes('@link')) {
        const url = item.header.join('').match(/@link\s*(.*)}/)
        if (url) item.url = url[1].trim()
      }

      if (node.type.includes('ExportAllDeclaration')) {
        return
      }

      if (node.type.includes('ExportDefaultDeclaration')) {
        return
      }

      if (node.type.includes('ExportNamedDeclaration')) {
        const firstDeclaration = node.declarations ? node.declarations[0] : node.declaration
        if (!firstDeclaration) return

        item.type = firstDeclaration.type || item.type

        if (item.type === 'VariableDeclaration') {
          item.name = node.declaration.declarations[0].id.name
        } else {
          item.name = node.declaration.id.name
        }

        if (node.declaration.superClass) {
          item.name = `\`${item.name}\` (extends \`${node.declaration.superClass.name}\`)`
        }

        if (item.type === 'FunctionDeclaration') {
          item.params = [] // node.declaration.params
          item.signature = []
        }
      }

      if (node.type.includes('MethodDefinition')) {
        item.name = node.key?.name
        item.signature = []

        if (node.value.type === 'FunctionExpression') {
          item.generator = node.value.generator
          item.static = node.static
          item.async = node.value.async
          item.params = []
          item.returns = []
        }
      }

      if (node.type === 'Property') {
        item.name = node.key?.name
        item.signature = []

        if (node.value.type === 'FunctionExpression') {
          item.generator = node.value.generator
          item.static = node.static
          item.async = node.value.async
          item.signature = []
          item.params = []
          item.returns = []
        }
      }

      if (item.export && !item.header) {
        item.header = [
          `This is a \`${item.type}\` named \`${item.name}\` ` +
          `in \`${srcFile}\`, it's exported but undocumented.\n`
        ]
      }

      const attrs = item.header?.join('\n').match(/@(.*)[\n$]*/g)

      if (attrs) {
        let position = 0

        for (const attr of attrs) {
          const isParam = attr.match(/^@(param|arg|argument)/)
          const isReturn = attr.match(/^@(returns?)/)

          if (isParam) {
            const propType = 'params'
            item.signature = item.signature || []
            const parts = attr.split(/-\s+(.*)/)
            const { 1: rawType, 2: rawName } = parts[0].match(/{([^}]+)}(.*)/)
            const [name, defaultValue] = rawName.replace(/[[\]']+/g, '').trim().split('=')

            // type could be [(string|number)=]
            const parenthasisedType = rawType
              .replace(/\s*\|\s*/g, ' \\| ')
              .replace(/\[|\]/g, '')
            // now it is (string|number)=
            const optional = parenthasisedType.endsWith('=')
            const compundType = parenthasisedType.replace(/=$/, '')
            // now it is (string|number)
            const type = compundType.match(/^\((.*)\)$/)?.[1] ?? compundType
            // now it is string|number

            const param = {
              name: name.trim() || `(Position ${position++})`,
              type
            }

            const params = node.declaration?.params || node.value?.params
            if (params) {
              const assign = params.find(o => o.left?.name === name)
              if (assign) param.default = assign.right.raw
            }

            param.default = defaultValue?.trim() ?? ''
            param.optional = optional
            param.desc = parts[1]?.trim()

            if (!item[propType]) item[propType] = []
            item[propType].push(param)
            if (propType === 'params' && !name.includes('.')) item.signature.push(name)
          }

          if (isReturn) {
            const propType = 'returns'
            const match = attr.match(/{([^}]+)}(?:\s*-\s*)?(.*)/)
            if (match) {
              const { 1: rawType, 2: rawName } = match
              const [name, description] = /\w\s*-\s*(.*)/.test(rawName) ? rawName.split(/-\s+/) : ['Not specified', rawName]
              const type = rawType.replace(/\s*\|\s*/g, ' \\| ')
              const param = { name: name.trim() || 'Not specified', type, description: description?.trim() }
              if (['undefined', 'void'].includes(type)) continue
              if (!item[propType]) item[propType] = []
              item[propType].push(param)
            }
          }
        }
      }

      if (item.signature && item.type !== 'ClassDeclaration') {
        item.name = `\`${item.name}(${item.signature?.join(', ') || ''})\``
      } else if (item.exports) {
        item.name = `\`${item.name}\``
      }

      if (item.header) {
        item.header = item.header.join('\n').split('\n').filter(line => !line.trim().startsWith('@'))
        docs.push(item)
      }
    }

    walk.full(ast, onNode)
    docs.sort((a, b) => a.sort - b.sort)

    const createTableParams = arr => {
      if (!arr || !arr.length) return []

      const tableHeader = [
        '| Argument | Type | Default | Optional | Description |',
        '| :---     | :--- | :---:   | :---:    | :---        |'
      ].join('\n')

      let table = `${tableHeader}\n`

      for (const param of arr) {
        // let type = param.type || 'Unknown'
        // const desc = param.header?.join(' ')

        table += `| ${Object.values(param).join(' | ')} |\n`
      }

      return (table + '\n')
    }

    const createTableReturn = (arr) => {
      if (!arr?.length) return []

      const tableHeader = [
        '| Return Value | Type | Description |',
        '| :---         | :--- | :---        |'
      ].join('\n')

      let table = `${tableHeader}\n`

      for (const param of arr) {
        // let type = param.type || 'Unknown'
        // const desc = param.header?.join(' ')

        table += `| ${Object.values(param).join(' | ')} |\n`
      }

      return (table + '\n')
    }

    const gitBase = 'https://github.com/socketsupply/socket/blob/'

    for (const doc of docs) {
      let h = doc.export ? '##' : '###'
      if (doc.type === 'Module') h = '#'

      const title = `\n${h} [${doc.name}](${gitBase}${gitTagOrBranch}${doc.location})\n`
      const header = `${doc.header.join('\n')}\n`

      const md = [
        title ?? [],
        doc?.url ? `External docs: ${doc.url}\n` : [],
        header ?? [],
        createTableParams(doc?.params),
        createTableReturn(doc?.returns)
      ].flatMap(item => item).join('\n')

      fs.appendFileSync(destFile, md, { flags: 'a' })
    }
  }

  if (typeof filename === 'object') {
    fs.appendFileSync(destFile, filename.content, { flags: 'a' })
  }
}

[
  'application.js',
  'bluetooth.js',
  // 'bootstrap.js', // don't document this module yet
  {
    file: 'buffer.js',
    content: `
# [Buffer](https://github.com/socketsupply/socket/blob/master/api/buffer.js)

Buffer module is a [third party](https://github.com/feross/buffer) vendor module provided by Feross Aboukhadijeh and other contributors (MIT License),

External docs: https://nodejs.org/api/buffer.html
`
  },
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
].forEach(file => transform(file, JS_INTERFACE_DIR, 'README.md', gitTagOrBranch))

transform('index.js', SOCKET_NODE_DIR, 'API.md')

// Generate config.md
const startMarker = 'constexpr auto gDefaultConfig = R"INI('
const endMarker = ')INI";'

const src = path.relative(process.cwd(), 'src/cli/templates.hh')
const data = fs.readFileSync(src, 'utf8')

const startIndex = data.indexOf(startMarker)

const remainingData = data.slice(startIndex + startMarker.length)
const endIndex = remainingData.indexOf(endMarker)

if (startIndex === -1 || endIndex === -1) {
  console.error('Start or end marker not found')
}

const extractedText = remainingData.slice(0, endIndex)

const parseIni = (iniText) => {
  const sections = {}
  let currentSection = null
  let lastComment = ''
  let defaultValue = ''

  iniText.split(/\r?\n/).forEach((line) => {
    const trimmedLine = line.trim()
    if (trimmedLine.startsWith(';')) {
      if (trimmedLine.includes('default value:')) {
        defaultValue = trimmedLine.split('default value:')[1].trim()
      } else {
        lastComment += ' ' + trimmedLine.slice(1).trim()
      }
    } else if (trimmedLine.startsWith('[') && trimmedLine.endsWith(']')) {
      currentSection = trimmedLine.slice(1, -1)
      sections[currentSection] = []
      lastComment = ''
    } else if (currentSection) {
      const keyValue = trimmedLine.split('=')
      if (keyValue.length === 2) {
        sections[currentSection].push({
          key: keyValue[0].trim(),
          value: keyValue[1].trim(),
          defaultValue,
          description: lastComment
        })
        lastComment = ''
        defaultValue = ''
      }
    }
  })

  return sections
}

const createConfigMd = (sections) => {
  let md = '# Configuration\n'
  md += '\n'
  Object.entries(sections).forEach(([sectionName, settings]) => {
    md += `## Section \`${sectionName}\`\n`
    md += '\n'
    md += 'Key | Default Value | Description\n'
    md += ':--- | :--- | :---\n'
    settings.forEach(({ key, defaultValue, description }) => {
      md += `${key} | ${defaultValue} | ${description}\n`
    })
    md += '\n'
  })
  return md
}

const sections = parseIni(extractedText)
const md = createConfigMd(sections)
fs.writeFileSync('api/config.md', md)
