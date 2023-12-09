import { Path } from './path.js'
import * as posix from './posix.js'
import * as win32 from './win32.js'

import {
  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC,
  HOME
} from './well-known.js'

export * as default from './index.js'

export {
  posix,
  win32,
  Path,

  // well known
  RESOURCES,
  DOWNLOADS,
  DOCUMENTS,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC,
  HOME
}
