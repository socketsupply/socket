import { access, stat } from 'fs/promises'
import path from 'path'

export async function redirect (inputPath, basePath) {
  // Resolve the full path for easier handling
  const fullPath = path.join(basePath, inputPath)

  // 1. Try the given path if it's a file
  if (await isFile(fullPath)) {
    return stripBasePath(fullPath, basePath)
  }

  // 2. Try appending a `/` to the path and checking for an index.html
  const indexPath = path.join(fullPath, 'index.html')
  if (await exists(indexPath)) return stripBasePath(indexPath, basePath)

  // 3. Check if appending a .html file extension gives a valid file
  const htmlPath = `${fullPath}.html`
  if (await isFile(htmlPath)) return stripBasePath(htmlPath, basePath)

  // Special case for root path
  if (inputPath === '/') {
    const rootIndexPath = path.join(basePath, 'index.html')
    return await exists(rootIndexPath) ? '/index.html' : null
  }

  // If no valid path is found, return null
  return null
}

// Helper function to check if a path exists
async function exists (filePath) {
  try {
    await access(filePath)
    return true
  } catch {
    return false
  }
}

// Helper function to check if a path points to a file
async function isFile (filePath) {
  try {
    const fileStat = await stat(filePath)
    return fileStat.isFile()
  } catch {
    return false
  }
}

// Helper function to strip the basePath from the fullPath
function stripBasePath (fullPath, basePath) {
  return fullPath.replace(basePath, '')
}
