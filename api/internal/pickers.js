import { SecurityError } from '../errors.js'
import application from '../application.js'
import path from '../path.js'

import {
  createFileSystemDirectoryHandle,
  createFileSystemFileHandle,
  FileSystemHandle
} from '../fs/web.js'

/**
 * Key-value store for general usage by the file pickers"
 * @ignore
 */
export class Database {
  get (key) {
    try {
      return JSON.parse(globalThis.localStorage.getItem(`internal:pickers:${key}`))
    } catch {
      return globalThis.localStorage.getItem(`internal:pickers:${key}`)
    }
  }

  set (key, value) {
    return globalThis.localStorage.setItem(`internal:pickers:${key}`, JSON.stringify(value))
  }
}

/**
 * Internal database for pickers, such as mapping IDs to directory/file paths.
 * @ignore
 */
export const db = new Database()

/**
 * Throws a `SecurityError` if "user transient activation" has not occurred.
 * @ignore
 * @see {@link https://developer.mozilla.org/en-US/docs/Glossary/Transient_activation}
 * @param {string} which
 */
function requireUserActivation (which) {
  if (globalThis.navigator && 'userActivation' in globalThis.navigator) {
    // @ts-ignore
    if (globalThis.navigator.userActivation?.isActive === false) {
      throw new SecurityError(
        `The '${which}' request is not allowed by the user agent or the ` +
        'platform in the current content.'
      )
    }
  }
}

/**
 * Resolves file/directory picker "start in" path from `input` with an
 * optional `id` that maps to a knonw directory path.
 * @ignore
 * @param {string|FileSystemHandle} [input]
 * @param {string?} [id]
 * @return {string?}
 */
function resolveStartInPath (input = null, id = null) {
  if (input instanceof FileSystemHandle && input.kind === 'directory') {
    return input.name
  }

  switch (input) {
    case 'resources': return path.RESOURCES
    case 'desktop': return path.DESKTOP
    case 'documents': return path.DOCUMENTS
    case 'downloads': return path.DOWNLOADS
    case 'music': return path.MUSIC
    case 'pictures': return path.PICTURES
    case 'videos': return path.VIDEOS
  }

  if (id) {
    return db.get(id) || null
  }

  return null
}

/**
 * Normalizes 'ipc://window.showFileSystemPicker' options given to
 * @ignore
 * @param {object} options
 * @param {string=} [options.id = null]
 * @param {boolean=} [options.files = false]
 * @param {boolean=} [options.multiple = false]
 * @param {boolean=} [options.directories = false]
 * @param {string=} [options.suggestedName = null]
 * @param {boolean=} [options.excludeAcceptAllOption = false]
 * @param {string|FileSystemHandle=} [options.startIn = null]
 * @param {Array<{ description?: string, accept: object }=>} [options.types = null]
 * @return {{
 *   contentTypeSpecs: string,
 *   allowMultiple: boolean,
 *   defaultName: string,
 *   defaultPath: string,
 *   allowFiles: boolean,
 *   allowDirs: boolean
 * }}
 */
function normalizeShowFileSystemPickerOptions (options) {
  const contentTypeSpecs = []
  const excludeAcceptAllOption = (
    options?.excludeAcceptAllOption === true ||
    (Array.isArray(options?.types) && options.types.length > 0)
  )

  // <mime>:<ext>,<ext>|...
  if (excludeAcceptAllOption === true && Array.isArray(options?.types)) {
    for (const type of options.types) {
      if (type && type?.accept && typeof type?.accept === 'object') {
        for (const mime in type.accept) {
          const extensions = type.accept[mime]
          if (Array.isArray(extensions)) {
            const normalized = extensions.map((e) => e.replace(/^\./, ''))
            contentTypeSpecs.push(`${mime}:${normalized.join(',')}`)
          }
        }
      }
    }
  }

  return {
    contentTypeSpecs: contentTypeSpecs.join('|'),
    allowMultiple: options?.multiple === true,
    defaultName: options?.suggestedName ?? '',
    defaultPath: resolveStartInPath(options?.startIn, options?.id) ?? '',
    // eslint-disable-next-line
    allowFiles: options?.files === true ? true : false,
    // eslint-disable-next-line
    allowDirs: options?.directories === true ? true : false
  }
}

/**
 * @typedef {{
 *   id?: string,
 *   mode?: 'read' | 'readwrite'
 *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
 * }} ShowDirectoryPickerOptions
 */

/**
 * Shows a directory picker which allows the user to select a directory.
 * @param {ShowDirectoryPickerOptions=} [options]
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showDirectoryPicker}
 * @return {Promise<FileSystemDirectoryHandle[]>}
 */
export async function showDirectoryPicker (options = null) {
  requireUserActivation('showDirectoryPicker')

  const { startIn, id } = options || {}
  const currentWindow = await application.getCurrentWindow()
  const [dirname] = await currentWindow.showDirectoryFilePicker(
    normalizeShowFileSystemPickerOptions({
      directories: true,
      multiple: false,
      files: false,
      startIn,
      id
    })
  )

  if (typeof id === 'string' && id.length > 0) {
    db.set(id, dirname)
  }

  return await createFileSystemDirectoryHandle(dirname)
}

/**
 * @typedef {{
 *   id?: string,
 *   excludeAcceptAllOption?: boolean,
 *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
 *   types?: Array<{
 *     description?: string,
 *     [keyof object]?: string[]
 *   }>
 * }} ShowOpenFilePickerOptions
 */

/**
 * Shows a file picker that allows a user to select a file or multiple files
 * and returns a handle for each selected file.
 * @param {ShowOpenFilePickerOptions=} [options]
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showOpenFilePicker}
 * @return {Promise<FileSystemFileHandle[]>}
 */
export async function showOpenFilePicker (options = null) {
  requireUserActivation('showOpenFilePicker')

  const { multiple, startIn, id, types, excludeAcceptAllOption } = options || {}
  const currentWindow = await application.getCurrentWindow()
  const filenames = await currentWindow.showOpenFilePicker(
    normalizeShowFileSystemPickerOptions({
      directories: false,
      files: true,

      excludeAcceptAllOption,
      multiple,
      startIn,
      types,
      id
    })
  )

  if (filenames.length === 1 && typeof id === 'string' && id.length > 0) {
    db.set(id, filenames[0])
  }

  const handles = []

  for (const filename of filenames) {
    handles.push(await createFileSystemFileHandle(filename, { writable: false }))
  }

  return handles
}

/**
 * @typedef {{
 *   id?: string,
 *   excludeAcceptAllOption?: boolean,
 *   suggestedName?: string,
 *   startIn?: FileSystemHandle | 'desktop' | 'documents' | 'downloads' | 'music' | 'pictures' | 'videos',
 *   types?: Array<{
 *     description?: string,
 *     [keyof object]?: string[]
 *   }>
 * }} ShowSaveFilePickerOptions
 */

/**
 * Shows a file picker that allows a user to save a file by selecting an
 * existing file, or entering a name for a new file.
 * @param {ShowSaveFilePickerOptions=} [options]
 * @see {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/showSaveFilePicker}
 * @return {Promise<FileSystemHandle>}
 */
export async function showSaveFilePicker (options = null) {
  requireUserActivation('showSaveFilePicker')

  const { startIn, id, types, excludeAcceptAllOption } = options || {}
  const currentWindow = await application.getCurrentWindow()
  const [filename] = await currentWindow.showSaveFilePicker(
    normalizeShowFileSystemPickerOptions({
      directories: false,
      multiple: false,
      files: true,

      excludeAcceptAllOption,
      startIn,
      types,
      id
    })
  )

  if (typeof id === 'string' && id.length > 0) {
    db.set(id, filename)
  }

  return await createFileSystemFileHandle(filename)
}

export default {
  showDirectoryPicker,
  showOpenFilePicker,
  showSaveFilePicker
}
