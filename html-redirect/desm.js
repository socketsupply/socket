import { fileURLToPath } from 'url'
import { dirname, join } from 'path'

function urlDirname (url) {
  return dirname(fileURLToPath(url))
}

function urlJoin (url, ...str) {
  return join(urlDirname(url), ...str)
}

export default urlDirname

export {
  fileURLToPath as filename,
  urlJoin as join,
  urlDirname as dirname
}
