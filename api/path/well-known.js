import ipc from '../ipc.js'

const paths = ipc.sendSync('os.paths')?.data ?? {}

/**
 * Well known path to the user's "Downloads" folder.
 * @type {?string}
 */
export const DOWNLOADS = paths.downloads || null

/**
 * Well known path to the user's "Documents" folder.
 * @type {?string}
 */
export const DOCUMENTS = paths.documents || null

/**
 * Well known path to the user's "Pictures" folder.
 * @type {?string}
 */
export const PICTURES = paths.pictures || null

/**
 * Well known path to the user's "Desktop" folder.
 * @type {?string}
 */
export const DESKTOP = paths.desktop || null

/**
 * Well known path to the user's "Videos" folder.
 * @type {?string}
 */
export const VIDEOS = paths.videos || null

/**
 * Well known path to the user's "Music" folder.
 * @type {?string}
 */
export const MUSIC = paths.music || null

/**
 * Well known path to the application's "resources" folder.
 * @type {?string}
 */
export const RESOURCES = paths.resources || null

/**
 * Well known path to the application's "config" folder.
 * @type {?string}
 */
export const CONFIG = paths.config || null

/**
 * Well known path to the application's "data" folder.
 * @type {?string}
 */
export const DATA = paths.data || null

/**
 * Well known path to the application's "log" folder.
 * @type {?string}
 */
export const LOG = paths.log || null

/**
 * Well known path to the application's "home" folder.
 * This may be the user's HOME directory or the application container sandbox.
 * @type {?string}
 */
export const HOME = paths.home || null

export default {
  DOWNLOADS,
  DOCUMENTS,
  RESOURCES,
  PICTURES,
  DESKTOP,
  VIDEOS,
  CONFIG,
  MUSIC,
  HOME,
  DATA,
  LOG
}
