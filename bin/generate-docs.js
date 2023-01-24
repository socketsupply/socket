#!/usr/bin/env node
import * as acorn from 'acorn'
import * as walk from 'acorn-walk'
import fs from 'node:fs'
import path from 'node:path'

try {
  fs.unlinkSync('README.md')
} catch {}

export function transform (filename) {
  const srcFile = path.relative(process.cwd(), filename)
  const destFile = path.relative(process.cwd(), 'README.md')

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
            const { 1: type, 2: rawName } = match
            const [name, description] = /\w\s*-\s*(.*)/.test(rawName) ? rawName.split(/-\s+/) : ['Not specified', rawName]
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

  const base = 'https://github.com/socketsupply/socket-api/blob/master'

  for (const doc of docs) {
    let h = doc.export ? '##' : '###'
    if (doc.type === 'Module') h = '#'

    const title = `\n${h} [${doc.name}](${base}${doc.location})\n`
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

[
  'bluetooth.js',
  'buffer.js',
  'crypto.js',
  'dgram.js',
  'dns/index.js',
  'dns/promises.js',
  'events.js',
  'fs/index.js',
  'fs/promises.js',
  'fs/stream.js',
  'ipc.js',
  'os.js',
  'p2p.js',
  'path/path.js',
  'process.js',
  'runtime.js',
  'stream.js'
].forEach(transform)
