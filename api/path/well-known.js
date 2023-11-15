import ipc from '../ipc.js'

const paths = ipc.sendSync('os.paths')

/**
 * Well known path to the user's "Downloads" folder.
 * @type {?string}
 */
export const DOWNLOADS = paths.data.downloads || null

/**
 * Well known path to the user's "Documents" folder.
 * @type {?string}
 */
export const DOCUMENTS = paths.data.documents || null

/**
 * Well known path to the user's "Pictures" folder.
 * @type {?string}
 */
export const PICTURES = paths.data.pictures || null

/**
 * Well known path to the user's "Desktop" folder.
 * @type {?string}
 */
export const DESKTOP = paths.data.desktop || null

/**
 * Well known path to the user's "Videos" folder.
 * @type {?string}
 */
export const VIDEOS = paths.data.videos || null

/**
 * Well known path to the user's "Music" folder.
 * @type {?string}
 */
export const MUSIC = paths.data.music || null

/**
 * Well known path to the application's "resources" folder.
 * @type {?string}
 */
export const RESOURCES = paths.data.resources || null

export default {
  DOWNLOADS,
  DOCUMENTS,
  RESOURCES,
  PICTURES,
  DESKTOP,
  VIDEOS,
  MUSIC
}
