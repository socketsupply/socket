/**
 * A map of known absolute file paths to file IDs that
 * have been granted access outside of the sandbox.
 * XXX(@jwerle): this is currently only used on linux, but valaues may
 * be added for all platforms, likely from a file system picker dialog.
 * @type {Map<string, string>}
 */
export const temporary = new Map()

export default {
  temporary
}
