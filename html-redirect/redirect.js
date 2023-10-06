import { access } from 'fs/promises'
import path from 'path'

export async function redirect (inputPath, basePath) {
  // Special case for root path
  if (inputPath === '/') return '/index.html'

  // Resolve the full path for easier handling
  const fullPath = path.join(basePath, inputPath)

  // 1. Try the given path
  console.log(1)
  if (await exists(fullPath)) {
    return stripBasePath(fullPath, basePath)
  }

  // 2. Try appending a `/` to the path and checking for an index.html
  console.log(2)
  const indexPath = path.join(fullPath, 'index.html')
  if (await exists(indexPath)) return stripBasePath(indexPath, basePath)

  // 3. Check if the input ends with .html, strip it and check
  console.log(3)
  if (path.extname(fullPath) === '.html') {
    const strippedPath = fullPath.substring(0, fullPath.length - 5)
    if (await exists(strippedPath)) return stripBasePath(strippedPath, basePath)
  }

  // 4. Check if a .html file exists for the given path
  console.log(4)
  const htmlPath = `${fullPath}.html`
  if (await exists(htmlPath)) return stripBasePath(htmlPath, basePath)

  // If no valid path is found, return null
  return null
}

// Helper function to check if a file or directory exists
async function exists (filePath) {
  try {
    await access(filePath)
    return true
  } catch {
    return false
  }
}

// Helper function to strip the basePath from the fullPath
function stripBasePath (fullPath, basePath) {
  return fullPath.replace(basePath, '')
}
