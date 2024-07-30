import { Path } from './path.js'
import * as posix from './posix.js'
import * as win32 from './win32.js'
import * as mounts from './mounts.js'
import * as exports from './index.js'
import {
  DOWNLOADS,
  DOCUMENTS,
  RESOURCES,
  PICTURES,
  DESKTOP,
  VIDEOS,
  CONFIG,
  MEDIA,
  MUSIC,
  HOME,
  DATA,
  LOG,
  TMP
} from './well-known.js'

export {
  mounts,
  posix,
  win32,
  Path,

  // well known paths
  DOWNLOADS,
  DOCUMENTS,
  RESOURCES,
  PICTURES,
  DESKTOP,
  VIDEOS,
  CONFIG,
  MEDIA,
  MUSIC,
  HOME,
  DATA,
  LOG,
  TMP
}

export default exports
