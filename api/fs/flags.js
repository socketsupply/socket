import {
  O_APPEND,
  O_RDONLY,
  O_WRONLY,
  O_TRUNC,
  O_CREAT,
  O_RDWR,
  O_EXCL,
  O_SYNC
} from './constants.js'

import * as exports from './flags.js'

export function normalizeFlags (flags) {
  if (typeof flags === 'number') {
    return flags
  }

  if (flags !== undefined && typeof flags !== 'string') {
    throw new TypeError(
      `Expecting flags to be a string or number: Got ${typeof flags}`
    )
  }

  switch (flags) {
    case 'r':
      return O_RDONLY

    case 'rs': case 'sr':
      return O_RDONLY | O_SYNC

    case 'r+':
      return O_RDWR

    case 'rs+': case 'sr+':
      return O_RDWR | O_SYNC

    case 'w':
      return O_TRUNC | O_CREAT | O_WRONLY

    case 'wx': case 'xw':
      return O_TRUNC | O_CREAT | O_WRONLY | O_EXCL

    case 'w+':
      return O_TRUNC | O_CREAT | O_RDWR

    case 'wx+': case 'xw+':
      return O_TRUNC | O_CREAT | O_RDWR | O_EXCL

    case 'a':
      return O_APPEND | O_CREAT | O_WRONLY

    case 'ax': case 'xa':
      return O_APPEND | O_CREAT | O_WRONLY | O_EXCL

    case 'as': case 'sa':
      return O_APPEND | O_CREAT | O_WRONLY | O_SYNC

    case 'a+':
      return O_APPEND | O_CREAT | O_RDWR

    case 'ax+': case 'xa+':
      return O_APPEND | O_CREAT | O_RDWR | O_EXCL

    case 'as+': case 'sa+':
      return O_APPEND | O_CREAT | O_RDWR | O_SYNC
  }

  return O_RDONLY
}

export default exports
