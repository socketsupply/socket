import fs from 'socket:fs/promises'
import path from 'socket:path'
import process from 'socket:process'

const preloadJavascript = `
__PRELOAD_JS_PLACEHOLDER__
`
export const getPreloadJavascript = () => {
  return preloadJavascript
}

const preloadScriptModule = `<script type="module">\n${preloadJavascript}\n</script>\n`

/**
 * @param {String} directory 
 * @param {(string): undefined} callback - Callback to invoke for each file entry in directory tree
 */
async function walk (directory, callback) {
  for (const entry of (await fs.readdir(directory, { withFileTypes: true }))) {
    const entryPath = path.join(directory, entry.name)
    if (entry.isDirectory()) {
      await walk(entryPath, callback)
    } else {
      await callback(entryPath)
    }
  }
}

const initSocketPreload = async () => {
  await walk(process.cwd(), async (filename) => {
    if (!filename.toLowerCase().endsWith('.html')) {
      return
    }
    
    let html = await fs.readFile(filename)
    if (html.indexOf('const preloadJavascript') < 0) {
      html = preloadScriptModule + html
      await fs.writeFile(filename, html)
    }
  })

  return preloadScriptModule.length
}

export default initSocketPreload
