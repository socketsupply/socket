function toKebabCase (inputString) {
  return inputString.replace(/([a-z])([A-Z])/g, '$1-$2').toLowerCase()
}

function parseIni (iniText) {
  const sections = {}

  const section = iniText.section === '' ? 'ssc' : `ssc ${iniText.section}`

  sections[section] = []

  let currentSubSection
  const description = []
  let option

  for (const line of iniText.extractedText.split('\n')) {
    const trimmedLine = line.trim()
    const isSubSection = /\w+:$/.test(trimmedLine) && trimmedLine[0] !== '-'

    // console.log('isSubsection', trimmedLine, isSubSection)

    if (trimmedLine.length === 0) {
      // Skip empty lines.
      continue
    }
    if (trimmedLine === 'ssc v{{ssc_version}}') {
      // skip version
      continue
    }
    if (line[0] !== ' ' && !isSubSection) {
      description.push(trimmedLine)
      continue
    } else {
      if (isSubSection) {
        // This line is a section heading.
        currentSubSection = trimmedLine.replace(/:$/, '')
        sections[section][currentSubSection] = []
      } else {
        if (!currentSubSection.includes('options') && currentSubSection !== 'subcommands') {
          sections[section][currentSubSection].push(trimmedLine)
        } else if (/.+\s{2,}.+/.test(trimmedLine)) {
          const [value, description] = trimmedLine.split(/\s{2,}/)
          option = value
          sections[section][currentSubSection][value] = [description]
        } else if (option) {
          sections[section][currentSubSection][option].push(trimmedLine)
        }
      }
    }
  }

  sections[section].description = description
  // console.log(sections[section])

  return sections
}

function generateIni (parts) {
  return parts.map(parseIni)
}

function createCliMd (sections) {
  let md = '# Command Line interface\n'
  md += 'These commands are available from the command line interface (CLI).\n'

  sections.forEach(section => {
    Object.entries(section).forEach(([key, value]) => {
      md += `# ${key}\n`
      const { description, usage, ...options } = value
      if (description) {
        md += description.join('\n') + '\n'
      }
      if (usage) {
        md += '## Usage\n'
        md += '```bash\n'
        md += usage.join('\n') + '\n'
        md += '```\n'
      }
      Object.entries(options).forEach(([key, value]) => {
        // create a table for each option
        md += `## ${key}\n`
        md += '| Option | Description |\n'
        md += '| --- | --- |\n'
        Object.entries(value).forEach(([key, value]) => {
          md += `| ${key} | ${value.join('<br>')} |\n`
        })
      })
    })
  })
  return md
}

// Generate CLI.md
export function generateCli (source) {
  const startMarker = /constexpr auto gHelpText(\S*) = R"TEXT\(/gm
  const endMarker = ')TEXT";'

  const sectionSources = [...source.matchAll(startMarker)].map(match => {
    const startIndex = match.index + match[0].length
    const remainingData = source.slice(startIndex)
    const endIndex = remainingData.indexOf(endMarker)
    const extractedText = remainingData.slice(0, endIndex)
    return { section: toKebabCase(match[1]), extractedText }
  })

  const sections = generateIni(sectionSources)
  return createCliMd(sections)
}
