import fs from 'socket:fs/promises'
import Path from 'socket:path'
import process from 'socket:process'

const preloadJavascript = `
/// PRELOAD_JS_PLACEHOLDER ///
`
export const getPreloadJavascript = () => {
  return preloadJavascript
}

const preloadScriptModule = `<script type="module">\n${preloadJavascript}\n</script>\n`

const recursePath = async (path, fileFunc, data) => {
  for (const entry of (await fs.readdir(path, { withFileTypes: true }))) {
    const entryPath = Path.join(path, entry.name)
    if (entry.isDirectory()) {
      await recursePath(entryPath, fileFunc, data)
    } else {
      await fileFunc(entryPath, data)
    }
  }
}

const initSocketPreload = async () => {
  const sourceDir = ''
  const destDir = ''

  await recursePath(process.cwd(), async (file) => {
    if (!(file.toLowerCase().endsWith('.html') || file.toLowerCase().endsWith('.htm'))) {
      return
    }
    const sourcePath = sourceDir.length > 0 ? Path.join(sourceDir, file) : file
    const destPath = destDir.length > 0 ? Path.join(destDir, file) : file
    let html = await fs.readFile(sourcePath)
    if (html.indexOf('const preloadJavascript') < 0) {
      html = preloadScriptModule + html
      await fs.writeFile(destPath, html)
    }
  })

  return preloadScriptModule.length
}

export default initSocketPreload
