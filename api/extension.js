/* global Event */
/* eslint-disable no-unused-vars */
/**
 * @module extension
 *
 * @example
 * import extension from 'socket:extension'
 * const ex = await extension.load('native_extension')
 * const result = ex.binding.method('argument')
 */
import { isBufferLike } from './util.js'
import { createFile } from './fs/web.js'
import application from './application.js'
// import { Buffer } from './buffer.js'
import process from './process.js'
import crypto from './crypto.js'
import ipc from './ipc.js'
import fs from './fs/promises.js'

/**
 * @typedef {number} {Pointer}
 */

const $loaded = Symbol('loaded')
const $stats = Symbol('stats')
const $type = Symbol('type')

const NULL = 0x0

// eslint-disable-next-line
const STDIN = 0x0 // STDIN_FILENO
const STDOUT = 0x1 // STDOUT_FILENO
const STDERR = 0x2 // STDERR_FILNO

const EXIT_FAILURE = 1
const EXIT_SUCCESS = 0
const RAND_MAX = (0x7fffffff)

const CLOCKS_PER_SEC = 1000

const TIME_UTC = 1
const TIMER_ABSTIME = 1

const CLOCK_REALTIME = 0
const CLOCK_MONOTONIC = 1
const CLOCK_PROCESS_CPUTIME_ID = 2
const CLOCK_THREAD_CPUTIME_ID = 3
const CLOCK_MONOTONIC_RAW = 4
const CLOCK_REALTIME_COARSE = 5
const CLOCK_MONOTONIC_COARSE = 6
const CLOCK_BOOTTIME = 7
const CLOCK_REALTIME_ALARM = 8
const CLOCK_BOOTTIME_ALARM = 9
const CLOCK_SGI_CYCLE = 10
const CLOCK_TAI = 11

const EPERM = 1
const ENOENT = 2
const ESRCH = 3
const EINTR = 4
const EIO = 5
const ENXIO = 6
const E2BIG = 7
const ENOEXEC = 8
const EBADF = 9
const ECHILD = 10
const EAGAIN = 11
const ENOMEM = 12
const EACCES = 13
const EFAULT = 14
const ENOTBLK = 15
const EBUSY = 16
const EEXIST = 17
const EXDEV = 18
const ENODEV = 19
const ENOTDIR = 20
const EISDIR = 21
const EINVAL = 22
const ENFILE = 23
const EMFILE = 24
const ENOTTY = 25
const ETXTBSY = 26
const EFBIG = 27
const ENOSPC = 28
const ESPIPE = 29
const EROFS = 30
const EMLINK = 31
const EPIPE = 32
const EDOM = 33
const ERANGE = 34
const EDEADLK = 35
const ENAMETOOLONG = 36
const ENOLCK = 37
const ENOSYS = 38
const ENOTEMPTY = 39
const ELOOP = 40
const EWOULDBLOCK = EAGAIN
const ENOMSG = 42
const EIDRM = 43
const ECHRNG = 44
const EL2NSYNC = 45
const EL3HLT = 46
const EL3RST = 47
const ELNRNG = 48
const EUNATCH = 49
const ENOCSI = 50
const EL2HLT = 51
const EBADE = 52
const EBADR = 53
const EXFULL = 54
const ENOANO = 55
const EBADRQC = 56
const EBADSLT = 57
const EDEADLOCK = EDEADLK
const EBFONT = 59
const ENOSTR = 60
const ENODATA = 61
const ETIME = 62
const ENOSR = 63
const ENONET = 64
const ENOPKG = 65
const EREMOTE = 66
const ENOLINK = 67
const EADV = 68
const ESRMNT = 69
const ECOMM = 70
const EPROTO = 71
const EMULTIHOP = 72
const EDOTDOT = 73
const EBADMSG = 74
const EOVERFLOW = 75
const ENOTUNIQ = 76
const EBADFD = 77
const EREMCHG = 78
const ELIBACC = 79
const ELIBBAD = 80
const ELIBSCN = 81
const ELIBMAX = 82
const ELIBEXEC = 83
const EILSEQ = 84
const ERESTART = 85
const ESTRPIPE = 86
const EUSERS = 87
const ENOTSOCK = 88
const EDESTADDRREQ = 89
const EMSGSIZE = 90
const EPROTOTYPE = 91
const ENOPROTOOPT = 92
const EPROTONOSUPPORT = 93
const ESOCKTNOSUPPORT = 94
const EOPNOTSUPP = 95
const ENOTSUP = EOPNOTSUPP
const EPFNOSUPPORT = 96
const EAFNOSUPPORT = 97
const EADDRINUSE = 98
const EADDRNOTAVAIL = 99
const ENETDOWN = 100
const ENETUNREACH = 101
const ENETRESET = 102
const ECONNABORTED = 103
const ECONNRESET = 104
const ENOBUFS = 105
const EISCONN = 106
const ENOTCONN = 107
const ESHUTDOWN = 108
const ETOOMANYREFS = 109
const ETIMEDOUT = 110
const ECONNREFUSED = 111
const EHOSTDOWN = 112
const EHOSTUNREACH = 113
const EALREADY = 114
const EINPROGRESS = 115
const ESTALE = 116
const EUCLEAN = 117
const ENOTNAM = 118
const ENAVAIL = 119
const EISNAM = 120
const EREMOTEIO = 121
const EDQUOT = 122
const ENOMEDIUM = 123
const EMEDIUMTYPE = 124
const ECANCELED = 125
const ENOKEY = 126
const EKEYEXPIRED = 127
const EKEYREVOKED = 128
const EKEYREJECTED = 129
const EOWNERDEAD = 130
const ENOTRECOVERABLE = 131
const ERFKILL = 132
const EHWPOISON = 133

const SAPI_JSON_TYPE_EMPTY = -1
const SAPI_JSON_TYPE_ANY = 0
const SAPI_JSON_TYPE_NULL = 1
const SAPI_JSON_TYPE_OBJECT = 2
const SAPI_JSON_TYPE_ARRAY = 3
const SAPI_JSON_TYPE_BOOLEAN = 4
const SAPI_JSON_TYPE_NUMBER = 5
const SAPI_JSON_TYPE_STRING = 6
const SAPI_JSON_TYPE_RAW = 7

const errorMessages = {
  0: 'Undefined error: 0',
  [EPERM]: 'Operation not permitted',
  [ENOENT]: 'No such file or directory',
  [ESRCH]: 'No such process',
  [EINTR]: 'Interrupted system call',
  [EIO]: 'Input/output error',
  [ENXIO]: 'Device not configured',
  [E2BIG]: 'Argument list too long',
  [ENOEXEC]: 'Exec format error',
  [EBADF]: 'Bad file descriptor',
  [ECHILD]: 'No child processes',
  [EAGAIN]: 'Try again',
  [ENOMEM]: 'Out of memory',
  [EACCES]: 'Permission denied',
  [EFAULT]: 'Bad address',
  [ENOTBLK]: 'Block device required',
  [EBUSY]: 'Device or resource busy',
  [EEXIST]: 'File exists',
  [EXDEV]: 'Cross-device link',
  [ENODEV]: 'No such device',
  [ENOTDIR]: 'Not a directory',
  [EISDIR]: 'Is a directory',
  [EINVAL]: 'Invalid argument',
  [ENFILE]: 'File table overflow',
  [EMFILE]: 'Too many open files',
  [ENOTTY]: 'Not a typewriter',
  [ETXTBSY]: 'Text file busy',
  [EFBIG]: 'File too large',
  [ENOSPC]: 'No space left on device',
  [ESPIPE]: 'Illegal seek',
  [EROFS]: 'Read-only file system ',
  [EMLINK]: 'Too many links',
  [EPIPE]: 'Broken pipe',
  [EDOM]: ' Numerical argument out of domain',
  [ERANGE]: 'umerical result out of range',
  [EDEADLK]: 'Resource deadlock avoided',
  [ENAMETOOLONG]: 'File name too long',
  [ENOLCK]: 'No locks available',
  [ENOSYS]: 'Function not implemented',
  [ENOTEMPTY]: 'Directory not empty',
  [ELOOP]: 'Too many levels of symbolic links',
  [EWOULDBLOCK]: 'Try again',
  [ENOMSG]: 'No message of desired type',
  [EIDRM]: 'Identifier removed',
  [ECHRNG]: 'Character range error',
  [EL2NSYNC]: 'Level 2 not synchronized',
  [EL3HLT]: 'Level 3 halted',
  [EL3RST]: 'EL3RST',
  [ELNRNG]: 'ELNRNG',
  [EUNATCH]: 'No such device',
  [ENOCSI]: 'No CSI structure available',
  [EL2HLT]: 'Level 2 halted',
  [EBADE]: 'Invalid exchange',
  [EBADR]: 'Invalid request descriptor',
  [EXFULL]: 'Exchange full',
  [ENOANO]: 'No anode',
  [EBADRQC]: 'Invalid request code',
  [EBADSLT]: 'Invalid slot',
  [EDEADLOCK]: 'Resource deadlock avoided',
  [EBFONT]: 'Bad font file format',
  [ENOSTR]: 'Device not a stream',
  [ENODATA]: 'No data available',
  [ETIME]: 'Timer expired',
  [ENOSR]: 'Out of streams resources',
  [ENONET]: 'Machine is not on the network',
  [ENOPKG]: 'Package not installed',
  [EREMOTE]: 'Object is remote',
  [ENOLINK]: 'Link has been severed',
  [EADV]: 'Advertise error',
  [ESRMNT]: 'Srmount error',
  [ECOMM]: 'Communication error on send',
  [EPROTO]: 'Protocol error',
  [EMULTIHOP]: 'Multihop attempted',
  [EDOTDOT]: 'RFS specific error',
  [EBADMSG]: 'Not a data message',
  [EOVERFLOW]: 'Value too large for defined data type',
  [ENOTUNIQ]: 'Name not unique on network',
  [EBADFD]: 'File descriptor in bad state',
  [EREMCHG]: 'Remote address changed',
  [ELIBACC]: 'Can not access a needed shared library',
  [ELIBBAD]: 'Accessing a corrupted shared library',
  [ELIBSCN]: '.lib section in a.out corrupted',
  [ELIBMAX]: 'Attempting to link in too many shared libraries',
  [ELIBEXEC]: 'Cannot exec a shared library directly',
  [EILSEQ]: 'Illegal byte sequence',
  [ERESTART]: 'Interrupted system call should be restarted',
  [ESTRPIPE]: 'Streams pipe error',
  [EUSERS]: 'Too many users',
  [ENOTSOCK]: 'Socket operation on non-socket',
  [EDESTADDRREQ]: 'Destination address required',
  [EMSGSIZE]: 'Message too long',
  [EPROTOTYPE]: 'Protocol wrong type for socket',
  [ENOPROTOOPT]: 'Protocol not available',
  [EPROTONOSUPPORT]: 'Protocol not supported',
  [ESOCKTNOSUPPORT]: 'Socket type not supported',
  [EOPNOTSUPP]: 'Operation not supported on transport endpoint',
  [ENOTSUP]: 'Operation not supported',
  [EPFNOSUPPORT]: 'Protocol family not supported',
  [EAFNOSUPPORT]: 'Address family not supported by protocol',
  [EADDRINUSE]: 'Address already in use',
  [EADDRNOTAVAIL]: 'Cannot assign requested address',
  [ENETDOWN]: 'Network is down',
  [ENETUNREACH]: 'Network is unreachable',
  [ENETRESET]: 'Network dropped connection because of reset',
  [ECONNABORTED]: 'Software caused connection abort',
  [ECONNRESET]: 'Connection reset by peer',
  [ENOBUFS]: 'No buffer space available',
  [EISCONN]: 'Transport endpoint is already connected',
  [ENOTCONN]: 'Transport endpoint is not connected',
  [ESHUTDOWN]: 'Cannot send after transport endpoint shutdown',
  [ETOOMANYREFS]: 'Too many references: cannot splice',
  [ETIMEDOUT]: 'Connection timed out',
  [ECONNREFUSED]: 'Connection refused',
  [EHOSTDOWN]: 'Host is down',
  [EHOSTUNREACH]: 'No route to host',
  [EALREADY]: 'Operation already in progress',
  [EINPROGRESS]: 'Operation now in progress',
  [ESTALE]: 'Stale NFS file handle',
  [EUCLEAN]: 'Structure needs cleaning',
  [ENOTNAM]: 'Not a XENIX named type file',
  [ENAVAIL]: 'No XENIX semaphores available',
  [EISNAM]: 'Is a named type file',
  [EREMOTEIO]: 'Remote I/O error',
  [EDQUOT]: 'Quota exceeded',
  [ENOMEDIUM]: 'No medium found',
  [EMEDIUMTYPE]: 'Wrong medium type',
  [ECANCELED]: 'Operation canceled',
  [ENOKEY]: 'Required key not available',
  [EKEYEXPIRED]: 'Key has expired',
  [EKEYREVOKED]: 'Key has been revoked',
  [EKEYREJECTED]: 'Key was rejected by service',
  [EOWNERDEAD]: 'Owner died',
  [ENOTRECOVERABLE]: 'State not recoverable',
  [ERFKILL]: 'Operation not possible due to RF-kill',
  [EHWPOISON]: 'Memory page has hardware error'
}

class WebAssemblyExtensionRuntimeObject {
  static get pointerSize () { return 4 }
  constructor (adapter) {
    this.adapter = adapter
  }
}

class WebAssemblyExtensionRuntimeRouter extends WebAssemblyExtensionRuntimeObject {
  routes = new Map()
}

class WebAssemblyExtensionRuntimePolicy extends WebAssemblyExtensionRuntimeObject {
  name = null
  allowed = false
}

class WebAssemblyExtensionRuntimeContext extends WebAssemblyExtensionRuntimeObject {
  // internal pointers
  data = NULL
  pointer = NULL

  pool = []
  json = new Map()
  error = null
  router = null
  config = Object.create(application.config)
  context = null
  retained = false
  policies = new Map()
  retainedCount = 0

  constructor (adapter, options) {
    super(adapter)

    this.context = options?.context ?? null

    if (Array.isArray(options?.policies)) {
      for (const policy of options.policies) {
        this.setPolicy(policy, true)
      }
    }

    this.router = this.context?.router ?? null
    this.confix = this.context?.config ? Object.create(this.context.config) : this.config
    this.retained = Boolean(options?.retained)
    this.policy = this.context?.policies ?? this.policies
    this.pointer = this.context
      ? this.context.alloc(WebAssemblyExtensionRuntimeObject.pointerSize)
      : this.adapter.heap.alloc(WebAssemblyExtensionRuntimeObject.pointerSize)

    this.adapter.contexts.set(this.pointer, this)
  }

  alloc (size) {
    const pointer = this.adapter.heap.alloc(size)

    if (pointer) {
      this.pool.push(pointer)
    }

    return pointer
  }

  allocJSON (value) {
    const pointer = this.alloc(4)
    this.json.set(pointer, value)
    return pointer
  }

  retain () {
    this.retained = true
    this.retainedCount++
  }

  release () {
    if (this.retainedCount === 0) {
      return false
    }

    if (--this.retainedCount === 0) {
      for (const pointer of this.pool) {
        const context = this.env.adapter.contexts(pointer)

        if (context) {
          context.release()
        }

        this.adapter.heap.free(pointer)
      }

      this.adapter.heap.free(this.pointer)
      this.pool.splice(0, this.pool.length)
      this.pointer = NULL
      this.retained = false
      this.context = null
      return true
    }

    return false
  }

  isAllowed (name) {
    const names = name.split(',')
    for (const name of names) {
      const parts = name.trim().split('_')
      let key = ''
      for (const part of parts) {
        key += part
        if (this.hasPolicy(key) && this.getPolicy(key).allowed) {
          return true
        }

        key += '_'
      }
    }

    return false
  }

  setPolicy (name, allowed) {
    if (this.hasPolicy(name)) {
      const policy = this.getPolicy(name)
      policy.name = name
      policy.allowed = Boolean(allowed)
    } else {
      this.policies.set(name, { name, allowed: Boolean(allowed) })
    }
  }

  getPolicy (name) {
    return this.policies.get(name) ?? null
  }
}

/**
 * A constructed `Response` suitable for WASM binaries.
 * @ignore
 */
class WebAssemblyResponse extends Response {
  constructor (stream) {
    super(stream, {
      headers: {
        'content-type': 'application/wasm'
      }
    })
  }
}

class WebAssemblyExtensionMemoryBlock {
  start = 0
  end = 0
  constructor (memory, start, end) {
    this.memory = memory
    this.start = start || 0
    this.end = end || memory.byteLength
  }
}

class WebAssemblyExtensionMemory {
  adapter = null
  offset = 0
  limits = { min: 0, max: 0 }

  constructor (adapter, min, max) {
    this.adapter = adapter
    this.limits.min = min
    this.limits.max = max
    this.offset = min
    this.current = []
    this.freeList = [new WebAssemblyExtensionMemoryBlock(this)]
  }

  get buffer () {
    return this.adapter.buffer
  }

  get byteLength () {
    return this.buffer.byteLength
  }

  get (pointer) {
    return this.buffer.subarray(pointer)
  }

  get pointer () {
    return this.offset
  }

  push (value) {
    let pointer = this.offset

    if (value === null) {
      value = 0
    }

    if (typeof value === 'string') {
      value = this.adapter.textEncoder.encode(value)
    } else if (isBufferLike(value)) {
      value = new Uint8Array(value)
    }

    if (!isBufferLike(value) && typeof value !== 'number') {
      throw new TypeError(
        'WebAssemblyExtensionMemory: ' +
        'Expected value to be TypedArray or a number: ' +
        `Got ${typeof value}`
      )
    }

    let byteLength = 0
    const isFloat = typeof value === 'number' && parseInt(value) !== value

    if (isBufferLike(value)) {
      byteLength = value.byteLength
      pointer = this.alloc(byteLength)
    } else {
      const zeroes = Math.clz32(value)
      if (zeroes <= 8) {
        byteLength = 4
      } else if (zeroes <= 16) {
        byteLength = 2
      } else if (zeroes <= 32) {
        byteLength = 1
      }
    }

    this.current.push(byteLength)
    this.offset += byteLength

    if (isBufferLike(value)) {
      this.adapter.set(pointer, value)
    } else if (byteLength === 4) {
      if (isFloat) {
        this.adapter.setFloat32(pointer, value)
      } else {
        this.adapter.setInt32(pointer, value)
      }
    } else if (byteLength === 2) {
      if (isFloat) {
        this.adapter.setFloat16(pointer, value)
      } else {
        this.adapter.setInt16(pointer, value)
      }
    } else if (byteLength === 1) {
      if (isFloat) {
        this.adapter.setFloat8(pointer, value)
      } else {
        this.adapter.setInt8(pointer, value)
      }
    }

    return pointer
  }

  pop () {
    if (this.current.length === 0) {
      return 0
    }

    const byteLength = this.current.pop()
    const pointer = this.offset - byteLength
    this.offset -= byteLength
    return pointer
  }

  restore (offset) {
    const pointers = []
    while (this.offset > offset) {
      pointers.push(this.pop())
    }
    return pointers
  }

  alloc (size) {
    if (!size) {
      return NULL
    }

    // find a free block that is large enough
    for (let i = 0; i < this.freeList.length; ++i) {
      const block = this.freeList[i]
      if (block.end - block.start >= size) {
        // allocate memory from the free block
        const allocatedBlock = new WebAssemblyExtensionMemoryBlock(
          this,
          block.start,
          block.start + size
        )

        // update the free block (if any remaining)
        if (block.end - allocatedBlock.end > 0) {
          block.start = allocatedBlock.end
        } else {
          // remove the block from the free list if no remaining space
          this.freeList.splice(i, 1)
        }

        this.offset = allocatedBlock.end
        const pointer = this.limits.min + allocatedBlock.start
        return pointer
      }
    }

    return NULL
  }

  free (pointer) {
    if (!pointer) {
      return
    }

    pointer = pointer - this.limits.min

    // find the block to free and merge adjacent free blocks
    for (let i = 0; i < this.freeList.length; ++i) {
      const block = this.freeList[i]
      if (pointer >= block.start && pointer < block.end) {
        // add the freed block back to the free list
        this.freeList.splice(i, 0, new WebAssemblyExtensionMemoryBlock(
          this,
          pointer,
          pointer + (block.end - block.start)
        ))

        // merge adjacent free blocks
        if (i < this.freeList.length - 1 && this.freeList[i + 1].start === block.end) {
          this.freeList[i].end = this.freeList[i + 1].end
          this.freeList.splice(i + 1, 1)
        }

        if (i > 0 && this.freeList[i - 1].end === block.start) {
          this.freeList[i - 1].end = block.end
          this.freeList.splice(i, 1)
        }

        this.offset = this.freeList[this.freeList.length - 1].start
        return
      }
    }
  }
}

class WebAssemblyExtensionStack extends WebAssemblyExtensionMemory {
  constructor (adapter) {
    super(
      adapter,
      adapter.instance.exports.__stack_low.value,
      adapter.instance.exports.__stack_high.value
    )
  }
}

class WebAssemblyExtensionHeap extends WebAssemblyExtensionMemory {
  constructor (adapter) {
    super(
      adapter,
      adapter.instance.exports.__heap_base.value,
      adapter.instance.exports.__heap_end.value
    )
  }

  realloc (pointer, size) {
    // if the pointer is NULL, it's equivalent to `alloc()`
    if (pointer === NULL) {
      return this.alloc(size)
    }

    // find the block corresponding to the given pointer
    for (let i = 0; i < this.freeList.length; ++i) {
      const block = this.freeList[i]
      if (pointer >= block.start && pointer < block.end) {
        const currentSize = block.end - block.start

        // if the new size is smaller or equal, return the existing pointer
        if (size <= currentSize) {
          return pointer
        }

        // try to extend the current block if there is enough space
        if (
          i < this.freeList.length - 1 &&
          this.freeList[i + 1].start - block.end >= size - currentSize
        ) {
          block.end += size - currentSize
          return pointer
        }

        // if extension is not possible, allocate a new block and copy the data
        const newPointer = this.alloc(size)
        const sourceArray = new Uint8Array(this.buffer.buffer, block.start, currentSize)
        const destinationArray = new Uint8Array(this.buffer.buffer, newPointer, currentSize)
        destinationArray.set(sourceArray)

        // free the original block
        this.free(pointer)

        return newPointer
      }
    }

    // return NULL if the pointer is not found (not allocated)
    return NULL
  }

  calloc (count, size) {
    const totalSize = count * size
    const pointer = this.alloc(totalSize)

    if (pointer === NULL) {
      // allocation failed
      return NULL
    }

    // initialize the allocated memory to zero
    const array = new Uint8Array(this.buffer.buffer, pointer, totalSize)
    array.fill(0)
    return pointer
  }
}

class WebAssemblyExtensionIndirectFunctionTable {
  constructor (adapter, options = null) {
    this.adapter = adapter
    this.table = (
      options?.table ??
      adapter.instance?.exports?.__indirect_function_table ??
      new WebAssembly.Table({ initial: 0, element: 'anyfunc' })
    )
  }

  call (index, ...args) {
    if (this.table.length === 0 || index >= this.table.length) {
      return null
    }

    const callable = this.table.get(index)
    const values = []
    const mark = this.adapter.stack.pointer

    for (const arg of args) {
      if (typeof arg === 'number') {
        values.push(arg)
      } else {
        values.push(this.adapter.stack.push(arg))
      }
    }

    const value = callable(...values)
    this.adapter.stack.restore(mark)

    return value
  }
}

/**
 * An adapter for reading and writing various values from a WebAssembly instance's
 * memory buffer.
 * @ignore
 */
class WebAssemblyExtensionAdapter {
  constructor ({ instance, module, table, memory, policies }) {
    this.view = new DataView(memory.buffer)
    this.table = table
    this.buffer = new Uint8Array(memory.buffer)
    this.module = module
    this.memory = memory
    this.policies = policies || []
    this.contexts = new Map() // mapping pointer address to instance
    this.instance = instance
    this.exitStatus = null
    this.textDecoder = new TextDecoder()
    this.textEncoder = new TextEncoder()
    this.errorMessagePointers = {}
    this.indirectFunctionTable = new WebAssemblyExtensionIndirectFunctionTable(this)
  }

  destroy () {
    this.view = null
    this.table = null
    this.buffer = null
    this.module = null
    this.memory = null
    this.instance = null
    this.textDecoder = null
    this.textEncoder = null
    this.indirectFunctionTable = null
    this.stack = null
    this.heap = null
    this.context = null
  }

  init () {
    this.stack = new WebAssemblyExtensionStack(this)
    this.heap = new WebAssemblyExtensionHeap(this)

    this.instance.exports.__wasm_call_ctors()
    const offset = this.instance.exports.__sapi_extension_initializer()
    this.context = new WebAssemblyExtensionRuntimeContext(this, {
      policies: this.policies || []
    })

    // preallocate error message pointertr
    for (const errorCode in errorMessages) {
      const errorMessage = errorMessages[errorCode]
      const pointer = this.heap.alloc(errorMessage.length)
      this.errorMessagePointers[errorCode] = pointer
      this.setString(pointer, errorMessage)
    }

    return Boolean(this.indirectFunctionTable.call(offset, this.context.pointer))
  }

  get globalBaseOffset () {
    return this.instance.exports.__global_base.value
  }

  get (pointer) {
    if (!pointer) {
      return null
    }

    return this.buffer.subarray(pointer)
  }

  set (pointer, value) {
    if (pointer) {
      if (typeof value === 'string') {
        value = this.textEncoder.encode(value)
      }

      this.buffer.set(value, pointer)
    }
  }

  getFloat32 (pointer) {
    return pointer ? this.view.getFloat32(pointer, true) : 0
  }

  setFloat32 (pointer, value) {
    return pointer ? (this.view.setFloat32(pointer, value, true), true) : false
  }

  getInt8 (pointer) {
    return pointer ? this.view.getInt8(pointer, true) : 0
  }

  setInt8 (pointer, value) {
    return pointer ? (this.view.setInt8(pointer, value, true), true) : false
  }

  getInt16 (pointer) {
    return pointer ? this.view.getInt16(pointer, true) : 0
  }

  setInt16 (pointer, value) {
    return pointer ? (this.view.setInt16(pointer, value, true), true) : false
  }

  getInt32 (pointer) {
    return pointer ? this.view.getInt32(pointer, true) : 0
  }

  setInt32 (pointer, value) {
    return pointer ? (this.view.setInt32(pointer, value, true), true) : false
  }

  getUint8 (pointer) {
    return pointer ? this.view.getUint8(pointer, true) : 0
  }

  setUint8 (pointer, value) {
    return pointer ? (this.view.setUint8(pointer, value, true), true) : false
  }

  getUint16 (pointer) {
    return pointer ? this.view.getUint16(pointer, true) : 0
  }

  setUint16 (pointer, value) {
    return pointer ? (this.view.setUint16(pointer, value, true), true) : false
  }

  getUint32 (pointer) {
    return pointer ? this.view.getUint32(pointer, true) : 0
  }

  setUint32 (pointer, value) {
    return pointer ? (this.view.setUint32(pointer, value, true), true) : false
  }

  getString (pointer, buffer, size) {
    if (!pointer) {
      return null
    }

    if (typeof buffer === 'number') {
      size = buffer
      buffer = null
    }

    const start = buffer
      ? buffer.slice(pointer)
      : this.buffer.subarray(pointer)

    const end = size || start.indexOf(0) // NULL byte

    if (end === -1) {
      return this.textDecoder.decode(start)
    }

    return this.textDecoder.decode(start.slice(0, end))
  }

  setString (pointer, string, buffer) {
    if (pointer) {
      const encoded = this.textEncoder.encode(string)
      ;(buffer || this.buffer).set(encoded, pointer)
      return true
    }

    return false
  }
}

/**
 * A container for a native WebAssembly extension info.
 * @ignore
 */
class WebAssemblyExtensionInfo {
  constructor (adapter) {
    this.adapter = adapter
  }

  get instance () {
    return this.adapter.instance
  }

  get abi () {
    return this.adapter.instance.exports.__sapi_extension_abi()
  }

  get name () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_name())
  }

  get version () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_version())
  }

  get description () {
    return this.adapter.getString(this.instance.exports.__sapi_extension_description())
  }
}

function createWebAssemblyExtensionBinding (adapter) {
  return new Proxy(adapter, {
    get (target, key) {
      if (typeof adapter.instance.exports[key] === 'function') {
        return adapter.instance.exports[key].bind(adapter.instance.exports)
      }

      // TODO: invoke IPC route?
    }
  })
}

function variadicFormattedestinationPointerringFromPointers (
  env,
  formatPointer,
  variadicArguments = 0x0,
  encoded = true
) {
  const format = env.adapter.getString(formatPointer)
  if (!variadicArguments) {
    if (encoded) {
      return env.adapter.textEncoder.encode(format)
    }

    return format
  }

  const buffer = env.adapter.buffer.slice(variadicArguments)
  const view = new DataView(buffer.buffer)

  let index = 0

  const regex = /%(l|ll|j|t|z)?(d|i|n|o|p|s|S|u|x|X|%)/g
  const output = format.replace(regex, (x) => {
    if (x === '%%') {
      return '%'
    }

    switch (x) {
      case '%S': case '%s': {
        const pointer = view.getInt32((index++) * 4, true)
        const string = env.adapter.getString(pointer)
        if (!string) {
          return '(null)'
        }

        return string
      }

      case '%llu': case '%lu':
      case '%lld': case '%ld':
      case '%zu':
      case '%i': case '%x': case '%X': case '%d': {
        return view.getInt32((index++) * 4, true)
      }

      case '%p': {
        return `0x${view.getUint32((index++) * 4, true).toString(16)}`
      }

      case '%u': {
        return view.getUint32((index++) * 4, true)
      }
    }

    return x
  })

  if (encoded) {
    return env.adapter.textEncoder.encode(output)
  }

  return output
}

function createWebAssemblyExtensionImports (env) {
  const imports = {
    env: {
      memoryBase: 5 * 1024 * 1024,
      tableBase: 0,
      memory: env.memory,
      table: env.table || new WebAssembly.Table({
        initial: 0,
        element: 'anyfunc'
      })
    }
  }

  let errnoPointer = 0
  let srandSeed = 1
  let atExitCallbackPointer = 0x0
  let exitCode = 0

  const toBeFlushed = {
    stdout: [],
    stderr: []
  }

  // <assert.h>
  Object.assign(imports.env, {
    __assert_fail (condition, filename, line, func) {
    }
  })

  // <errno.h>
  Object.assign(imports.env, {
    __errno_location () {
      if (!errnoPointer) {
        errnoPointer = env.adapter.heap.alloc(4)
      }

      return errnoPointer
    }
  })

  // <stdio.h>
  Object.assign(imports.env, {
    printf (formatPointer, variadicArguments) {
      return imports.env.fprintf(STDOUT, formatPointer, variadicArguments)
    },

    fprintf (descriptorPointer, formatPointer, variadicArguments) {
      const output = variadicFormattedestinationPointerringFromPointers(
        env,
        formatPointer,
        variadicArguments,
        /* encoded = */ false
      )

      if (descriptorPointer === STDOUT) {
        toBeFlushed.stdout.push(output)
      } else if (descriptorPointer === STDERR) {
        toBeFlushed.stderr.push(output)
      }

      if (output.includes('\n')) {
        if (descriptorPointer === STDOUT) {
          console.log(toBeFlushed.stdout.join(''))
          toBeFlushed.stdout.splice(0, toBeFlushed.stdout.length)
        } else if (descriptorPointer === STDERR) {
          console.error(toBeFlushed.stderr.join(''))
          toBeFlushed.stderr.splice(0, toBeFlushed.stderr.length)
        }
      }

      return output.length
    },

    sprintf (stringPointer, formatPointer, variadicArguments) {
      const output = variadicFormattedestinationPointerringFromPointers(
        env,
        formatPointer,
        variadicArguments
      )

      if (stringPointer !== 0) {
        const buffer = env.adapter.get(stringPointer)
        buffer.set(output)
      }

      return output.byteLength
    },

    snprintf (stringPointer, size, formatPointer, variadicArguments) {
      const output = variadicFormattedestinationPointerringFromPointers(
        env,
        formatPointer,
        variadicArguments
      )

      if (stringPointer !== 0 && size !== 0) {
        const buffer = env.adapter.get(stringPointer)
        buffer.set(output.slice(0, size - 1))
      }

      return output.byteLength
    }
  })

  // <stdlib.h>
  Object.assign(imports.env, {
    /**
     * @param {number} stringPointer
     * @return {number}
     */
    atoi (stringPointer) {
      if (!stringPointer) {
        return 0
      }

      const string = env.adapter.getString(stringPointer)

      if (!string) {
        return 0
      }

      return parseInt(string.trim()) || 0
    },

    /**
     * @param {number} stringPointer
     * @return {number}
     */
    atol (stringPointer) {
      return imports.env.atoi(stringPointer)
    },

    /**
     * @param {number} stringPointer
     * @return {number}
     */
    atoll (stringPointer) {
      return imports.env.atoi(stringPointer)
    },

    /**
     * @param {number} stringPointer
     * @return {number}
     */
    atof (stringPointer) {
      if (!stringPointer) {
        return 0
      }

      const string = env.adapter.getString(stringPointer)

      if (!string) {
        return 0
      }

      return parseFloat(string) || 0
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @return {number}
     */
    strtof (stringPointer, endPointer = 0) {
      if (!stringPointer) {
        return 0
      }

      const string = env.adapter.getString(stringPointer)

      if (!string) {
        return 0
      }

      const regex = /^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$/
      const match = regex.exec(string.trim())

      if (match) {
        const value = parseFloat(match[0])
        return value
      }

      return Number.NaN
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @return {number}
     */
    strtod (stringPointer, endPointer) {
      return imports.env.strtof(stringPointer, endPointer)
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @return {number}
     */
    strtold (stringPointer, endPointer) {
      return imports.env.strtof(stringPointer, endPointer)
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @param {number} base
     * @return {number}
     */
    strtol (stringPointer, endPointer, base) {
      if (!stringPointer) {
        return 0
      }

      const string = env.adapter.getString(stringPointer)

      if (!string) {
        return 0
      }

      return parseInt(string, base) || 0
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @param {number} base
     * @return {number}
     */
    strtoul (stringPointer, endPointer, base) {
      return imports.env.strol(stringPointer, endPointer, base)
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @param {number} base
     * @return {number}
     */
    strtoll (stringPointer, endPointer, base) {
      return imports.env.strol(stringPointer, endPointer, base)
    },

    /**
     * TODO(@jwerle): handle `endPointer`
     * @param {number} stringPointer
     * @param {number=} [endPointer]
     * @param {number} base
     * @return {number}
     */
    strtoull (stringPointer, endPointer, base) {
      return imports.env.strol(stringPointer, endPointer, base)
    },

    /**
     * @return {number}
     */
    rand () {
      const x = Math.sin(srandSeed++) * 10000
      const y = x - Math.floor(x)
      // map the random number to the interval [0, RAND_MAX]
      return Math.floor(y * (RAND_MAX + 1))
    },

    /**
     * @param {number} seed
     */
    srand (seed) {
      // convert to unsigned integer
      srandSeed = (new Uint32Array([seed]))[0]
    },

    /**
     * @param {number} size
     * @return {number}
     */
    malloc (size) {
      return env.adapter.heap.alloc(size)
    },

    /**
     * @param {number} count
     * @param {number} size
     * @return {number}
     */
    calloc (count, size) {
      return env.adapter.heap.calloc(count, size)
    },

    /**
     * @param {number} pointer
     * @param {number} size
     * @return {number}
     */
    realloc (pointer, size) {
      return env.adapter.heap.realloc(pointer, size)
    },

    /**
     * @param {number} pointer
     */
    free (pointer) {
      return env.adapter.heap.free(pointer)
    },

    /**
     */
    abort () {
      throw new WebAssembly.Exception(env.tags.abort, [])
    },

    /**
     * @param {number} callbackPointer
     */
    atexit (callbackPointer) {
      if (!callbackPointer) {
        return -1
      }

      atExitCallbackPointer = callbackPointer
      return 0
    },

    /**
     * @param {number} code
     */
    exit (code) {
      exitCode = code
      // TODO(@jerle): exit WASM runtime with exit code
      env.adapter.indirectFunctionTable.call(atExitCallbackPointer)
      throw new WebAssembly.Exception(env.tags.exit, [code])
    },

    /**
     * @param {number} code
     */
    _Exit (code) {
      env.imports.exit(code)
    },

    /**
     * @param {number} callbackPointer
     */
    at_quick_exit (callbackPointer) {
      return env.imports.atexit(callbackPointer)
    },

    /**
     * @param {number} code
     */
    quick_exit (code) {
      env.imports.exit(code)
    },

    /**
     * @param {number} namePointer
     * @return {number}
     */
    getenv (namePointer) {
      if (!namePointer) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return NULL
      }

      const name = env.adapter.getString(namePointer)

      if (!name) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return NULL
      }

      if (!process.env[name]) {
        return NULL
      }

      return env.adapter.stack.push(process.env[name])
    },

    /**
     * @param {number} namePointer
     * @param {number} valuePointer
     * @param {number} overwrite
     */
    setenv (namePointer, valuePointer, overwrite) {
      if (!namePointer || !valuePointer) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      const name = env.adapter.getString(namePointer)
      const value = env.adapter.getString(valuePointer)

      if (!name || !value) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      if (name in process.env) {
        if (overwrite) {
          process.env[name] = value
        }
      } else {
        process.env[name] = value
      }

      return 0
    },

    /**
     * @param {number} namePointer
     */
    unsetenv (namePointer) {
      if (!namePointer) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      const name = env.adapter.getString(namePointer)
      if (!name) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      delete process.env[name]
      return 0
    },

    /**
     * @param {number} stringPointer
     */
    putenv (stringPointer) {
      if (!stringPointer) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      const string = env.adapter.getString(stringPointer)

      if (!string) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      const parts = string.split('=')

      if (parts.length !== 2) {
        env.adapter.setUint8(imports.env.__errno_location(), EINVAL)
        return -1
      }

      const [name, value] = parts

      process.env[name] = value

      return 0
    },

    /**
     * @return {number}
     */
    clearenv () {
      console.warn('clearenv: Operation is ignored')
      return -1
    },

    /**
     * @param {number} namePointer
     * @return {number}
     */
    secure_getenv (namePointer) {
      return imports.env.getenv(namePointer)
    },

    /**
     * @param {number} stringPointer
     */
    system (stringPointer) {
      console.warn('system: Operation is ignored')
      return -1
    },

    /**
     * @param {number} value
     * @return {number}
     */
    abs (value) {
      return Math.abs(parseInt(value))
    },

    /**
     * @param {number} value
     * @return {number}
     */
    labs (value) {
      return imports.env.abs(value)
    },

    /**
     * @param {number} value
     * @return {number}
     */
    llabs (value) {
      return imports.env.abs(value)
    },

    /**
     * @param {number} outputPointer
     * @param {number} numerator
     * @param {numerator} denominator
     */
    div (outputPointer, numerator, denominator) {
      const result = {
        quot: Math.floor(numerator / denominator),
        rem: numerator % denominator
      }

      env.adapter.setInt8(outputPointer, result.quot)
      env.adapter.setInt8(outputPointer + 4, result.rem)
    },

    /**
     * @param {number} outputPointer
     * @param {number} numerator
     * @param {numerator} denominator
     */
    ldiv (outputPointer, numerator, denominator) {
      return imports.env.div(outputPointer, numerator, denominator)
    },

    /**
     * @param {number} outputPointer
     * @param {number} numerator
     * @param {numerator} denominator
     */
    lldiv (outputPointer, numerator, denominator) {
      return imports.env.div(outputPointer, numerator, denominator)
    }
  })

  // <string.h>
  Object.assign(imports.env, {
    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @param {number} size
     * @return {number}
     */
    memcpy (destinationPointer, sourcePointer, size) {
      if (!destinationPointer) {
        return NULL
      }

      if (sourcePointer && size) {
        const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer, size)
        const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer, size)
        destination.set(source)
      }

      return destinationPointer
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @param {number} size
     * @return {number}
     */
    memmove (destinationPointer, sourcePointer, size) {
      if (!destinationPointer) {
        return NULL
      }

      if (sourcePointer && size) {
        const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer, size)
        const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer, size)

        if (destinationPointer < sourcePointer) {
          // copy from beginning to end
          destination.set(source)
        } else if (destinationPointer > sourcePointer) {
          // copy from end to beginning
          for (let i = size - 1; i >= 0; --i) {
            destination[i] = source[i]
          }
        } // else they overlap
      }

      return destinationPointer
    },

    /**
     * @param {number} destinationPointer
     * @param {number} byte
     * @param {number} size
     * @return {number}
     */
    memset (destinationPointer, byte, size) {
      if (!destinationPointer) {
        return NULL
      }

      const destination = new Uint8Array(
        env.adapter.buffer.buffer,
        destinationPointer,
        size
      )

      destination.fill(byte)
      return destinationPointer
    },

    /**
     * @param {number} left
     * @param {number} right
     * @param {number} size
     * @return {number}
     */
    memcmp (left, right, size) {
      const view1 = new Uint8Array(env.adapter.buffer.buffer, left, size)
      const view2 = new Uint8Array(env.adapter.buffer.buffer, right, size)

      for (let i = 0; i < size; ++i) {
        if (view1[i] !== view2[i]) {
          return view1[i] - view2[i]
        }
      }

      // left and right are equal
      return 0
    },

    /**
     * @param {number} stringPointer
     * @param {number} byte
     * @param {number} size
     * @return {number}
     */
    memchr (stringPointer, byte, size) {
      const view = new Uint8Array(env.adapter.buffer.buffer, stringPointer, size)

      for (let i = 0; i < size; ++i) {
        if (view[i] === byte) {
          // return the pointer where the character can be found
          return stringPointer + i
        }
      }

      return 0
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @return {number}
     */
    strcpy (destinationPointer, sourcePointer) {
      const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer)
      const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer)
      let i = 0

      for (; source[i] !== 0; ++i) {
        destination[i] = source[i]
      }

      // "null-terminate" the destination string
      destination[i] = 0
      return destinationPointer
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @param {number} size
     * @return {number}
     */
    strncpy (destinationPointer, sourcePointer, size) {
      const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer)
      const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer)
      let i = 0

      while (i < size && source[i] !== 0) {
        destination[i] = source[i]
        i++
      }

      // Pad the remaining characters with null if necessary
      while (i < size) {
        destination[i] = 0
        i++
      }

      return destinationPointer
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @return {number}
     */
    strcat (destinationPointer, sourcePointer) {
      const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer)
      const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer)
      let i = 0
      let j = 0

      while (destination[i] !== 0) {
        i++
      }

      // copy the source string to the end of the destination string
      while (source[j] !== 0) {
        destination[i + j] = source[j]
        j++
      }

      // "null-terminate" the concatenated string
      destination[i + j] = 0

      return destinationPointer
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @param {number} size
     * @return {number}
     */
    strncat (destinationPointer, sourcePointer, size) {
      const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer)
      const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer)
      let i = 0
      let j = 0

      while (destination[i] !== 0) {
        i++
      }

      // copy the source string up to `size` characters to
      // the end of the destination string
      while (j < size && source[j] !== 0) {
        destination[i + j] = source[j]
        j++
      }

      // "null-terminate" the concatenated string
      destination[i + j] = 0
      return destinationPointer
    },

    /**
     * @param {number} left
     * @param {number} right
     * @return {number}
     */
    strcmp (left, right) {
      const view1 = new Uint8Array(env.adapter.buffer.buffer, left)
      const view2 = new Uint8Array(env.adapter.buffer.buffer, right)
      let i = 0

      while (view1[i] !== 0 && view1[i] === view2[i]) {
        i++
      }

      return view1[i] - view2[i]
    },

    /**
     * @param {number} left
     * @param {number} right
     * @param {number} size
     * @return {number}
     */
    strncmp (left, right, size) {
      const view1 = new Uint8Array(env.adapter.buffer.buffer, left)
      const view2 = new Uint8Array(env.adapter.buffer.buffer, right)
      let i = 0

      while (i < size && view1[i] !== 0 && view1[i] === view2[i]) {
        i++
      }

      if (i === size || (view1[i] === 0 && view2[i] === 0)) {
        // strings are equal up to the specified size or both terminated
        return 0
      }

      return view1[i] - view2[i]
    },

    /**
     * @param {number} left
     * @param {number} right
     * @return {number}
     */
    strcoll (left, right) {
      const view1 = new Uint8Array(env.adapter.buffer.buffer, left)
      const view2 = new Uint8Array(env.adapter.buffer.buffer, right)
      const string1 = String.fromCharCode(...view1)
      const string2 = String.fromCharCode(...view2)

      return string1.localeCompare(string2)
    },

    /**
     * @param {number} destinationPointer
     * @param {number} sourcePointer
     * @param {number} size
     * @return {number}
     */
    strxfrm (destinationPointer, sourcePointer, size) {
      const destination = new Uint8Array(env.adapter.buffer.buffer, destinationPointer)
      const source = new Uint8Array(env.adapter.buffer.buffer, sourcePointer)
      const string = String.fromCharCode.apply(null, source)

      // use `Intl.Collator` for locale-sensitive comparison
      const collator = new Intl.Collator(globalThis.navigator.language)
      // normalize and remove diacritics
      const transformed = string
        .normalize('NFD')
        .replace(/[\u0300-\u036f]/g, '')

      // convert to plain byte array
      const bytes = Array.from(string).map((char) => char.charCodeAt(0))

      // copy transformed string to destination
      for (let i = 0; i < bytes.length; ++i) {
        destination[i] = bytes[i]
      }

      // "null-terminate" the transformed string
      destination[bytes.length] = 0

      // return the length of the transformed string
      return bytes.length
    },

    strxfrm_l (destinationPointer, sourcePointer, size, locale) {
      // TODO@(jwerle): figure out how to work with `locale_t`
      console.warn('strxfrm_l: Operation is not supported')
      return -1
    },

    strchr (stringPointer, byte) {
      const view = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      let i = 0

      while (view[i] !== 0 && view[i] !== byte) {
        i++
      }

      // return the pointer if found,
      if (view[i] === byte) {
        return stringPointer + i
      }

      return NULL
    },

    strrchr (stringPointer, byte) {
      const view = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      let lastOccurrence = 0

      for (let i = 0; view[i] !== 0; ++i) {
        if (view[i] === byte) {
          lastOccurrence = stringPointer + i
        }
      }

      return lastOccurrence
    },

    strcspn (stringPointer, charsetPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      const charset = new Uint8Array(env.adapter.buffer.buffer, charsetPointer)
      let i = 0

      while (string[i] !== 0) {
        let j = 0

        while (charset[j] !== 0 && charset[j] !== string[i]) {
          j++
        }

        if (charset[j] !== 0) {
          return i
        }

        i++
      }

      return i
    },

    strspn (stringPointer, charsetPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      const charset = new Uint8Array(env.adapter.buffer.buffer, charsetPointer)
      let i = 0

      while (string[i] !== 0) {
        let j = 0

        while (charset[j] !== 0 && charset[j] !== string[i]) {
          j++
        }

        if (charset[j] === 0) {
          return i
        }

        i++
      }

      return i
    },

    strpbrk (stringPointer, charsetPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      const charset = new Uint8Array(env.adapter.buffer.buffer, charsetPointer)

      for (let i = 0; i < string[i]; ++i) {
        for (let j = 0; charset[j] !== 0; ++j) {
          if (charset[j] === string[i]) {
            return stringPointer + i
          }
        }
      }

      // return `NULL` if no character from the set is found
      return 0
    },

    strstr (stringPointer, needlePointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      const needle = new Uint8Array(env.adapter.buffer.buffer, needlePointer)

      for (let i = 0; string[i] !== 0; ++i) {
        let match = true

        for (let j = 0; needle[j] !== 0; ++j) {
          if (string[i + j] !== needle[j]) {
            match = false
            break
          }
        }

        if (match) {
          return string + i
        }
      }

      // return `NULL` if the substring is not found
      return 0
    },

    strtok (stringPointer, delimitersPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      const delimiters = new Uint8Array(env.adapter.buffer.buffer, delimitersPointer)

      // save the position of the next token in linear memory
      let tokenPointer = stringPointer

      // find the start of the next token
      while (
        string[tokenPointer - stringPointer] !== 0 &&
        delimiters.includes(string[tokenPointer - stringPointer])
      ) {
        tokenPointer++
      }

      // if the end of the string is reached, return `NULL~
      if (string[tokenPointer - stringPointer] === 0) {
        return NULL
      }

      // save the position of the start of the token
      const tokenStartPointer = tokenPointer

      // find the end of the token
      while (
        string[tokenPointer - stringPointer] !== 0 &&
        !delimiters.includes(string[tokenPointer - stringPointer])
      ) {
        tokenPointer++
      }

      // if the end of the string is reached, update the next token position to 0
      if (string[tokenPointer - stringPointer] === 0) {
        tokenPointer = 0
      } else {
        // "null-terminate" the token
        string[tokenPointer - stringPointer] = 0
      }

      return tokenStartPointer
    },

    strlen (stringPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      let size = 0

      while (string[size] !== 0) {
        size++
      }

      return size
    },

    strdup (stringPointer) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      let size = 0

      while (string[size] !== 0) {
        size++
      }

      // allocate memory for the new string
      const pointer = env.adapter.heap.alloc(size + 1) // `+1` for the "null terminator"
      const duplicate = new Uint8Array(env.adapter.buffer.buffer, pointer)

      // copy the original string to the new memory location
      for (let i = 0; i <= size; ++i) {
        duplicate[i] = string[i]
      }

      return pointer
    },

    strndup (stringPointer, maxSize) {
      const string = new Uint8Array(env.adapter.buffer.buffer, stringPointer)
      let size = 0

      while (string[size] !== 0 && size < maxSize) {
        size++
      }

      // allocate memory for the new string
      const pointer = env.adapter.heap.alloc(size + 1) // `+1` for the "null terminator"
      const duplicate = new Uint8Array(env.adapter.buffer.buffer, pointer)

      // copy the original string to the new memory location
      for (let i = 0; i <= size; ++i) {
        duplicate[i] = string[i]
      }

      return pointer
    },

    strerror (errorCode) {
      if (errorCode >= 0 && errorCode in errorMessages) {
        const errorMessagePointer = env.adapter.errorMessagePointers[errorCode]

        if (!errorMessagePointer) {
          return NULL
        }

        const errorMessage = new Uint8Array(env.adapter.buffer.buffer, errorMessagePointer)

        // copy the error message to the allocated memory
        for (let i = 0; i < errorMessages[errorCode].length; ++i) {
          errorMessage[i] = errorMessages[errorCode].charCodeAt(i)
        }

        // null-terminate the string
        errorMessage[errorMessages[errorCode].length] = 0
        return errorMessagePointer
      }

      return NULL
    },

    strsignal (signal) {
      console.warn('strsignal: Operation is not supported')
      return NULL
    }
  })

  // <time.h>
  Object.assign(imports.env, {
    time (timePointer) {
      const now = Date.now() * 1000 // convert to seconds
      if (timePointer !== 0) {
        env.adapter.setUint32(timePointer, now)
      }
      return now
    },

    difftime (leftTime, rightTime) {
      return rightTime - leftTime
    },

    mktime (outputPointer, tmPointer) {
    },

    strftime (stringPointer, maxSize, formatPointer, tmPointer) {
    },

    strptime (bufferPointer, formatPointer, tmPointer) {
    },

    gmtime (timePointer) {
    },

    timegm (tmPointer) {
    },

    localtime (timePointer) {
    },

    asctime (tmPointer) {
    },

    ctime (timePointer) {
    },

    stime (timePointer) {
    },

    timespec_get (timeSpecPointer, base) {
    },

    getdate (stringPointer) {
    },

    nanosleep (rqtp, tmtp) {
    },

    gmtime_r (timePointer, tmPointer) {
    },

    localtime_r (timePointer, tmPointer) {
    },

    asctime_r (tmPointer, outputPointer) {
    },

    time_r (timePointer, outputPointer) {
    },

    // clock
    clock () {
    },

    clock_getres (clockId, timeSpecPointer) {
    },

    clock_gettime (clockId, timeSpecPointer) {
    },

    clock_nanosleep (clockId, flags, requestTimeSpecPointer, remainTimeSpecPointer) {
    }
  })

  // <ulimit.h>
  Object.assign(imports.env, {
    ulimit (commandValue, varargs) {
      console.warn('strxfrm_l: Operation is not supported')

      return -1
    }
  })

  // <unistd.h>
  Object.assign(imports.env, {
  })

  // <uv.h>
  Object.assign(imports.env, {
  })

  // <socket/extension.h>
  Object.assign(imports.env, {
    // utils
    sapi_log (ctx, message) {
      const string = env.adapter.getString(message)
      console.log(string)
    },

    sapi_debug (ctx, message) {
      const string = env.adapter.getString(message)
      console.debug(string)
    },

    sapi_rand64 () {
      return crypto.rand64()
    },

    // extension
    sapi_extension_register (registrationPointer) {
      console.warn('sapi_extension_register: Operation is not supported')
    },

    sapi_extension_is_allowed (contextPointer, allowedPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          const string = env.adapter.getString(allowedPointer)
          if (string) {
            return context.isAllowed(string)
          }
        }
      }

      return false
    },

    sapi_extension_load (contextPointer, namePointer, dataPointer) {
      console.warn(': Operation is not supported')
    },

    sapi_extension_unload (contextPointer, namePointer) {
      console.warn(': Operation is not supported')
    },

    sapi_extension_get (contextPointer, namePointer) {
      console.warn(': Operation is not supported')
    },

    // context
    sapi_context_create (parentPointer, retained) {
      let parent = null

      if (parentPointer) {
        parent = env.adapter.contexts.get(parentPointer) ?? null
      }

      const context = new WebAssemblyExtensionRuntimeContext(env.adapter, {
        context: parent,
        retained
      })

      return context.pointer
    },

    sapi_context_retain (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          context.retain()
          return true
        }
      }

      return false
    },

    sapi_context_retained (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.retained
        }
      }

      return false
    },

    sapi_context_get_loop (context) {
      return NULL
    },

    sapi_context_release (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.release()
        }
      }

      return false
    },

    sapi_context_set_data (contextPointer, dataPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          context.data = dataPointer
          return true
        }
      }

      return false
    },

    sapi_context_get_data (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.data
        }
      }

      return NULL
    },

    sapi_context_dispatch (context, data, callback) {
      if (callback > 0) {
        setTimeout(() => {
          try {
            env.adapter.indirectFunctionTable.call(callback, context, data)
          } catch (err) {
            if (err.is(env.tags.exit)) {
              env.adapter.exitStatus = err.getArg(env.tags.exit, 0)
              env.adapter.destroy()
            } else if (err.is(env.tags.abort)) {
              env.adapter.exitStatus = 1
              env.adapter.destroy()
            } else {
              throw err
            }
          }
        })
      }
    },

    sapi_context_alloc (contextPointer, size) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.alloc(size)
        }
      }

      return NULL
    },

    sapi_context_get_parent (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context?.context?.pointer) {
          return context.context.pointer
        }
      }

      return NULL
    },

    sapi_context_error_set_code (contextPointer, code) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          if (!context.error) {
            context.error = new Error()
          }
          context.error.code = code
        }
      }
    },

    sapi_context_error_get_code (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context?.error) {
          return context.error.code || 0
        }
      }

      return 0
    },

    sapi_context_error_set_name (contextPointer, namePointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          const name = env.adapter.getString(namePointer)
          if (name) {
            if (!context.error) {
              context.error = new Error()
            }
            context.error.name = name
          }
        }
      }
    },

    sapi_context_error_get_name (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context?.error) {
          return context.error.name || NULL
        }
      }

      return NULL
    },

    sapi_context_error_set_message (contextPointer, messagePointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          const message = env.adapter.getString(messagePointer)
          if (message) {
            if (!context.error) {
              context.error = new Error()
            }
            context.error.message = message
          }
        }
      }
    },

    sapi_context_error_get_message (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context?.error) {
          return context.error.message || NULL
        }
      }

      return NULL
    },

    sapi_context_error_set_location (contextPointer, locationPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          const location = env.adapter.getString(locationPointer)
          if (location) {
            if (!context.error) {
              context.error = new Error()
            }
            context.error.location = location
          }
        }
      }
    },

    sapi_context_error_get_location (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context?.error) {
          return context.error.location || NULL
        }
      }

      return NULL
    },

    sapi_context_config_get (contextPointer, keyStringPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        const string = env.adapter.getString(keyStringPointer)
        if (context && string) {
          const value = context.config[string]
          if (value) {
            return env.adapter.stack.push(value)
          }
        }
      }

      return NULL
    },

    sapi_context_config_set (contextPointer, keyStringPointer, valueStringPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        const string = env.adapter.getString(keyStringPointer)
        if (context && string) {
          const value = env.adapter.getString(valueStringPointer)
          if (value) {
            context.config[string] = value
          }
        }
      }
    },

    // env
    sapi_env_get (contextPointer, name) {
      if (!contextPointer) {
        return NULL
      }

      return imports.env.getenv(name)
    },

    // javascript
    async sapi_javascript_evaluate (context, name, source) {
      name = env.adapter.getString(name)
      source = env.adapter.getString(source)
      // TODO(@jwerle): implement a 'vm' module and use it
      // eslint-disable-next-line
      const evaluation = new Function(`
        return function () { ${source}; }
        //# sourceURL=${name}
      `)()
      try {
        await evaluation()
      } catch (err) {
        console.error('sapi_javascript_evaluate:', err)
      }
    },

    // JSON
    sapi_json_typeof (valuePointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(valuePointer)) {
          const value = context.json.get(valuePointer)
          if (value === null) {
            return SAPI_JSON_TYPE_NULL
          }

          if (Array.isArray(value)) {
            return SAPI_JSON_TYPE_ARRAY
          }

          if (typeof value === 'string') {
            return SAPI_JSON_TYPE_STRING
          }

          if (typeof value === 'number') {
            return SAPI_JSON_TYPE_NUMBER
          }

          if (typeof value === 'boolean') {
            return SAPI_JSON_TYPE_BOOLEAN
          }

          if (typeof value === 'object') {
            return SAPI_JSON_TYPE_OBJECT
          }

          return SAPI_JSON_TYPE_ANY
        }
      }

      return SAPI_JSON_TYPE_EMPTY
    },

    sapi_json_object_create (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.allocJSON({})
        }
      }

      return NULL
    },

    sapi_json_array_create (contextPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.allocJSON([])
        }
      }

      return NULL
    },

    sapi_json_string_create (contextPointer, stringPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        const string = env.adapter.getString(stringPointer)
        if (context && string) {
          return context.allocJSON(string)
        }
      }

      return NULL
    },

    sapi_json_boolean_create (contextPointer, booleanValue) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.allocJSON(Boolean(booleanValue))
        }
      }

      return NULL
    },

    sapi_json_number_create (contextPointer, numberValue) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        if (context) {
          return context.allocJSON(numberValue)
        }
      }

      return NULL
    },

    sapi_json_raw_from (contextPointer, stringPointer) {
      if (contextPointer) {
        const context = env.adapter.contexts.get(contextPointer)
        const string = env.adapter.getString(stringPointer)
        if (context && string) {
          return context.allocJSON(JSON.parse(string))
        }
      }

      return NULL
    },

    sapi_json_object_set_value (objectPointer, keyStringPointer, valuePointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(objectPointer)) {
          const object = context.json.get(objectPointer)
          const key = env.adapter.getString(keyStringPointer)
          if (object && key) {
            for (const context of env.adapter.contexts.values()) {
              if (context.json.has(valuePointer)) {
                const value = context.json.get(valuePointer)
                object[key] = value
                return
              }
            }
          }
        }
      }
    },

    sapi_json_object_get (objectPointer, keyStringPointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(objectPointer)) {
          const object = context.json.get(objectPointer)
          const key = env.adapter.getString(keyStringPointer)
          if (object && key && key in object) {
            const value = object[key]
            for (const entry of context.json.entries()) {
              if (entry[1] === value) {
                return entry[0]
              }
            }
          }
        }
      }

      return NULL
    },

    sapi_json_array_set_value (arrayPointer, index, valuePointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(arrayPointer)) {
          const array = context.json.get(arrayPointer)
          if (array) {
            for (const context of env.adapter.contexts.values()) {
              if (context.json.has(valuePointer)) {
                const value = context.json.get(valuePointer)
                array[index] = value
                return
              }
            }
          }
        }
      }
    },

    sapi_json_array_get (arrayPointer, index) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(arrayPointer)) {
          const array = context.json.get(arrayPointer)
          if (array) {
            const value = array[index]
            for (const entry of context.json.entries()) {
              if (entry[1] === value) {
                return entry[0]
              }
            }
          }
        }
      }

      return NULL
    },

    sapi_json_array_push_value (arrayPointer, valuePointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(arrayPointer)) {
          const array = context.json.get(arrayPointer)
          if (array) {
            const index = array.length
            for (const context of env.adapter.contexts.values()) {
              if (context.json.has(valuePointer)) {
                const value = context.json.get(valuePointer)
                array[index] = value
                return index
              }
            }
          }
        }
      }

      return -1
    },

    sapi_json_array_pop (arrayPointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(arrayPointer)) {
          const array = context.json.get(arrayPointer)
          if (array) {
            const value = array.pop()
            for (const entry of context.json.entries()) {
              if (entry[1] === value) {
                return entry[0]
              }
            }
          }
        }
      }

      return NULL
    },

    sapi_json_stringify_value (valuePointer) {
      for (const context of env.adapter.contexts.values()) {
        if (context.json.has(valuePointer)) {
          const value = context.json.get(valuePointer)
          return env.adapter.stack.push(JSON.stringify(value))
        }
      }

      return NULL
    },

    sapi_ipc_message_get_index (messagePointer) {
    },

    sapi_ipc_message_get_value (messagePointer) {
    },

    sapi_ipc_message_get_bytes (messagePointer) {
    },

    sapi_ipc_message_get_bytes_size (messagePointer) {
    },

    sapi_ipc_message_get_name (messagePointer) {
    },

    sapi_ipc_message_get_seq (messagePointer) {
    },

    sapi_ipc_message_get_uri (messagePointer) {
    },

    sapi_ipc_message_get (messagePointer, keyStringPointer) {
    },

    sapi_ipc_message_clone (contextPointer, messagePointer) {
    },

    sapi_ipc_result_create (contextPointer, messagePointer) {
    },

    sapi_ipc_result_clone (contextPointer, resultPointer) {
    },

    sapi_ipc_result_set_seq (resultPointer, seqPointer) {
    },

    sapi_ipc_result_get_seq (resultPointer) {
    },

    sapi_ipc_result_get_context (resultPointer) {
    },

    sapi_ipc_result_set_message (resultPointer, messagePointer) {
    },

    sapi_ipc_result_get_message (resultPointer) {
    },

    sapi_ipc_result_set_json (resultPointer, jsonPointer) {
    },

    sapi_ipc_result_get_json (resultPointer) {
    },

    sapi_ipc_result_set_json_data (result, jsonPointer) {
    },

    sapi_ipc_result_get_json_data (resultPointer) {
    },

    sapi_ipc_result_set_json_error (resultPointer, jsonPointer) {
    },

    sapi_ipc_result_get_json_error (resultPointer) {
    },

    sapi_ipc_result_set_bytes (resultPointer, size, bytesPointer) {
    },

    sapi_ipc_result_get_bytes (resultPointer) {
    },

    sapi_ipc_result_get_bytes_size (resultPointer) {
    },

    sapi_ipc_result_set_header (resultPointer, namePointer, valuePointer) {
    },

    sapi_ipc_result_get_header (resultPointer, namePointer) {
    },

    sapi_ipc_result_from_json (contextPointer, messagePointer, jsonPointer) {
    },

    sapi_ipc_send_chunk (resultPointer, chunkPointer, chunkSize, finished) {
    },

    sapi_ipc_send_event (resultPointer, namePointer, dataPointer, finished) {
    },

    sapi_ipc_set_cancellation_handler (resultPointer, handlerPointer, dataPointer) {
    },

    sapi_ipc_reply (resultPointer) {
    },

    sapi_ipc_reply_with_error (resultPointer, errorPointer) {
    },

    sapi_ipc_send_json (contextPointer, messagePointer, jsonPointer) {
    },

    sapi_ipc_send_json_with_result (contextPointer, resultPointer, jsonPointer) {
    },

    sapi_ipc_send_bytes (contextPointer, messagePointer, size, bytesPointer, headersPointer) {
    },

    sapi_ipc_send_bytes_with_result (contextPointer, resultPointer, size, bytesPointer, headersPointer) {
    },

    sapi_ipc_emit (contextPointer, namePointer, dataPointer) {
    },

    sapi_ipc_invoke (contextPointer, urlPointer, size, bytesPointer, callbackPointer) {
    },

    sapi_ipc_router_map (contextPointer, routePointer, callbackPointer, dataPointer) {
    },

    sapi_ipc_router_unmap (contextPointer, routePointer) {
    },

    sapi_ipc_router_listen (contextPointer, routePointer, callbackPointer, dataPointer) {
    },

    sapi_ipc_router_unlisten (contextPointer, routePointer, token) {
    },

    sapi_process_exec (contextPointer, commandPointer) {
      console.warn('sapi_process_exec: Operation is not supported')
      return NULL
    },

    sapi_process_exec_get_exit_code (processPointer) {
      console.warn('sapi_process_exec_get_exit_code: Operation is not supported')
      return -1
    },

    sapi_process_exec_get_output (processPointer) {
      console.warn('sapi_process_exec_get_output: Operation is not supported')
      return NULL
    },

    sapi_process_spawn (
      contextPointer,
      commandPointer,
      argvPointer,
      pathPointer,
      onstdoutPointer,
      onstderrPointer,
      onexitPointer
    ) {
      console.warn('sapi_process_spawn: Operation is not supported')
      return NULL
    },

    sapi_process_spawn_get_exit_code (processPointer) {
      console.warn('sapi_process_spawn_get_exit_code: Operation is not supported')
      return -1
    },

    sapi_process_spawn_get_pid (processPointer) {
      console.warn('sapi_process_spawn_get_pid: Operation is not supported')
      return -1
    },

    sapi_process_spawn_get_context (processPointer) {
      console.warn('sapi_process_spawn_get_context: Operation is not supported')
      return NULL
    },

    sapi_process_spawn_wait (processPointer) {
      console.warn('sapi_process_spawn_wait: Operation is not supported')
      return -1
    },

    sapi_process_spawn_write (processPointer, bytesPointer, size) {
      console.warn('sapi_process_spawn_write: Operation is not supported')
      return false
    },

    sapi_process_spawn_close_stdin (processPointer) {
      console.warn('sapi_process_spawn_close_stdin: Operation is not supported')
      return false
    },

    sapi_process_spawn_kill (processPointer, code) {
      console.warn('sapi_process_spawn_kill: Operation is not supported')
      return false
    }
  })

  return imports
}

/**
 * @typedef {{
 *   allow: string[] | string,
 *   imports?: object,
 *   type?: 'shared' | 'wasm32',
 *   instance?: WebAssembly.Instance,
 *   adapter?: WebAssemblyExtensionAdapter
 * }} ExtensionLoadOptions
 */

/**
 * @typedef {{ abi: number, version: string, description: string }} ExtensionInfo
 */

/**
 * @typedef {{ abi: number, loaded: number }} ExtensionStats
 */

/**
 * A interface for a native extension.
 * @template {Record<string, any> T}
 */
export class Extension extends EventTarget {
  /**
   * Load an extension by name.
   * @template {Record<string, any> T}
   * @param {string} name
   * @param {ExtensionLoadOptions} [options]
   * @return {Promise<Extension<T>>}
   */
  static async load (name, options) {
    options = { name, ...options }

    if (Array.isArray(options.allow)) {
      options.allow = options.allow.join(',')
    }

    options.type = await this.type(name)
    const stats = await this.stats(name)

    let info = null

    if (options.type === 'wasm32') {
      let adapter = null

      const path = stats.path.startsWith('/') ? stats.path.slice(1) : stats.path
      const fd = await fs.open(path)
      const file = await createFile(path, { fd })
      const stream = file.stream()
      const response = new WebAssemblyResponse(stream)
      const table = new WebAssembly.Table({
        initial: 0,
        element: 'anyfunc'
      })

      const tags = {
        ...options.tags,
        exit: new WebAssembly.Tag({ parameters: ['i32'] }),
        abort: new WebAssembly.Tag({ parameters: [] })
      }

      const memory = options.memory ?? new WebAssembly.Memory({ initial: 32 })
      const imports = {
        ...options.imports,
        ...createWebAssemblyExtensionImports({
          tags,
          table,
          memory,
          get adapter () {
            return adapter
          }
        })
      }

      const {
        instance,
        module
      } = await WebAssembly.instantiateStreaming(response, imports)

      adapter = new WebAssemblyExtensionAdapter({
        policies: options.allowed || [],
        instance,
        memory,
        module,
        table
      })

      info = new WebAssemblyExtensionInfo(adapter)

      options.adapter = adapter
      options.instance = instance

      try {
        adapter.init()
      } catch (err) {
        if (typeof err?.is === 'function') {
          if (err.is(tags.exit)) {
            adapter.exitStatus = err.getArg(tags.exit, 0)
            adapter.destroy()
          } else if (err.is(tags.abort)) {
            adapter.exitStatus = 1
            adapter.destroy()
          } else {
            throw err
          }
        } else {
          throw err
        }
      }
    } else {
      const result = await ipc.request('extension.load', options)
      if (result.err) {
        throw new Error('Failed to load extensions', { cause: result.err })
      }

      info = result.data
    }

    const extension = new this(name, info, options)
    extension[$stats] = stats
    extension[$loaded] = true
    return extension
  }

  /**
   * Query type of extension by name.
   * @param {string} name
   * @return {Promise<'shared'|'wasm32'|'unknown'|null>}
   */
  static async type (name) {
    const result = await ipc.request('extension.type', { name })

    if (result.err) {
      throw result.err
    }

    return result.data.type ?? null
  }

  /**
   * Provides current stats about the loaded extensions or one by name.
   * @param {?string} name
   * @return {Promise<ExtensionStats|null>}
   */
  static async stats (name) {
    const result = await ipc.request('extension.stats', { name: name || '' })
    if (result.err) {
      throw result.err
    }
    return result.data ?? null
  }

  /**
   * The name of the extension
   * @type {string?}
   */
  name = null

  /**
   * The version of the extension
   * @type {string?}
   */
  version = null

  /**
   * The description of the extension
   * @type {string?}
   */
  description = null

  /**
   * The abi of the extension
   * @type {number}
   */
  abi = 0

  /**
   * @type {object}
   */
  options = {}

  /**
   * @type {T}
   */
  binding = null

  /**
   * Not `null` if extension is of type 'wasm32'
   * @type {?WebAssemblyExtensionAdapter}
   */
  adapter = null

  /**
   * `Extension` class constructor.
   * @param {string} name
   * @param {ExtensionInfo} info
   * @param {ExtensionLoadOptions} [options]
   */
  constructor (name, info, options = null) {
    super()

    this[$type] = options?.type || 'shared'
    this[$loaded] = false

    this.abi = info?.abi || 0
    this.name = name
    this.version = info?.version || null
    this.description = info?.description || null
    this.options = options ?? {}

    if (this.type === 'shared') {
      this.binding = ipc.createBinding(options?.binding?.name || this.name, {
        ...options?.binding,
        extension: this,
        default: 'request'
      })
    } else if (this.type === 'wasm32') {
      this.adapter = options?.adapter
      this.binding = createWebAssemblyExtensionBinding(this)
    }
  }

  /**
   * `true` if the extension was loaded, otherwise `false`
   * @type {boolean}
   */
  get loaded () {
    return this[$loaded]
  }

  /**
   * The extension type: 'shared' or  'wasm32'
   * @type {'shared'|'wasm32'}
   */
  get type () {
    return this[$type]
  }

  /**
   * Unloads the loaded extension.
   * @throws Error
   */
  async unload () {
    if (!this.loaded) {
      throw new Error('Extension is not loaded')
    }

    if (this.type === 'shared') {
      const { name } = this
      const result = await ipc.request('extension.unload', { name })

      if (result.err) {
        throw new Error('Failed to unload extension', { cause: result.err })
      }
    } else if (this.type === 'wasm32') {
      // remove references
      this.instance = null
      this.adapter = null
      this.binding = null
    }

    this[$loaded] = false
    this.dispatchEvent(new Event('unload'))

    return true
  }
}

/**
 * Load an extension by name.
 * @template {Record<string, any> T}
 * @param {string} name
 * @param {ExtensionLoadOptions} [options]
 * @return {Promise<Extension<T>>}
 */
export async function load (name, options = {}) {
  return await Extension.load(name, options)
}

/**
 * Provides current stats about the loaded extensions.
 * @return {Promise<ExtensionStats>}
 */
export async function stats () {
  return await Extension.stats()
}

export default {
  load,
  stats
}
