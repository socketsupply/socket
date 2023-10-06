import { stat } from 'fs/promises'

export async function redirect(path) {
  // Assuming path is a url path segment to part of the FS that is available to load

  const direct = await exists(path)
  return null
}


async function exists (path) {
  try {
    if (await stat(path)) return true
    return false
  } catch (err) {
    return false
  }
}
