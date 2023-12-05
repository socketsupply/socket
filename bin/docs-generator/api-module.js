import * as acorn from 'acorn'
import * as walk from 'acorn-walk'

export function generateApiModuleDoc ({
  src,
  location,
  gitTagOrBranch = 'master'
}) {
  let accumulateComments = []
  const comments = {}
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
  let header = 'Unknown module'

  const onNode = node => {
    const item = {
      sort: node.loc.start.line,
      location: `/${location}#L${node.loc.start.line}`,
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
      if (name) {
        item.name = name[1]
        header = item.name
      }
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
        `in \`${location}\`, it's exported but undocumented.\n`
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
          const [name, defaultValue] = rawName.replace(/[[\]']+/g, '').trim().split(/ *[=] */)

          // type could be [(string|number)=]
          const parenthasisedType = rawType
            .replace(/\s*\|\s*/g, ' \\| ')
            .replace(/\[|\]/g, '')
          // now it is (string|number)=
          const optional = parenthasisedType.endsWith('=') || /^ *\[/.test(rawName)
          const compundType = parenthasisedType.replace(/=$/, '')
          // now it is (string|number)
          const type = compundType.match(/^\((.*)\)$/)?.[1] ?? compundType
          // now it is string|number

          const param = {
            name: name || `(Position ${position++})`,
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

  let content = '<!-- This file is generated by bin/docs-generator/api-module.js -->\n'
  content += '<!-- Do not edit this file directly. -->\n\n'

  for (const doc of docs) {
    let h = doc.export ? '##' : '###'
    if (doc.type === 'Module') h = '#'

    const title = `${h} [${doc.name}](${gitBase}${gitTagOrBranch}${doc.location})\n`
    const header = `${doc.header.join('\n')}\n`

    content += title ? `${title}\n` : ''
    content += doc?.url ? `External docs: ${doc.url}\n` : ''
    content += header ? `${header}\n` : ''
    content += createTableParams(doc?.params)
    content += createTableReturn(doc?.returns)
  }

  return { content, header }
}
