import ipc from '../ipc.js'

/**
 * A base container class for diagnostic information.
 */
export class Diagnostic {
  /**
   * A container for handles related to the diagnostics
   */
  static Handles = class Handles {
    /**
     * The nunmber of handles in this diagnostics.
     * @type {number}
     */
    count = 0

    /**
     * A set of known handle IDs
     * @type {string[]}
     */
    ids = []

    /**
     * `Diagnostic.Handles` class constructor.
     * @private
     */
    constructor () {
      Object.seal(this)
    }
  }

  /**
   * Known handles for this diagnostics.
   * @type {Diagnostic.Handles}
   */
  handles = new Diagnostic.Handles()
}

/**
 * A container for libuv diagnostics
 */
export class UVDiagnostic extends Diagnostic {
  /**
   * A container for libuv metrics.
   */
  static Metrics = class Metrics {
    /**
     * The number of event loop iterations.
     * @type {number}
     */
    loopCount = 0

    /**
     * Number of events that have been processed by the event handler.
     * @type {number}
     */
    events = 0

    /**
     * Number of events that were waiting to be processed when the
     * event provider was called.
     * @type {number}
     */
    eventsWaiting = 0
  }

  /**
   * Known libuv metrics for this diagnostic.
   * @type {UVDiagnostic.Metrics}
   */
  metrics = new UVDiagnostic.Metrics()

  /**
   * The current idle time of the libuv loop
   * @type {number}
   */
  idleTime = 0

  /**
   * The number of active requests in the libuv loop
   * @type {number}
   */
  activeRequests = 0
}

/**
 * A container for Core Post diagnostics.
 */
export class PostsDiagnostic extends Diagnostic {}

/**
 * A container for child process diagnostics.
 */
export class ChildProcessDiagnostic extends Diagnostic {}

/**
 * A container for AI diagnostics.
 */
export class AIDiagnostic extends Diagnostic {
  /**
   * A container for AI LLM diagnostics.
   */
  static LLMDiagnostic = class LLMDiagnostic extends Diagnostic {}

  /**
   * Known AI LLM diagnostics.
   * @type {AIDiagnostic.LLMDiagnostic}
   */
  llm = new AIDiagnostic.LLMDiagnostic()
}

/**
 * A container for various filesystem diagnostics.
 */
export class FSDiagnostic extends Diagnostic {
  /**
   * A container for filesystem watcher diagnostics.
   */
  static WatchersDiagnostic = class WatchersDiagnostic extends Diagnostic {}

  /**
   * A container for filesystem descriptors diagnostics.
   */
  static DescriptorsDiagnostic = class DescriptorsDiagnostic extends Diagnostic {}

  /**
   * Known FS watcher diagnostics.
   * @type {FSDiagnostic.WatchersDiagnostic}
   */
  watchers = new FSDiagnostic.WatchersDiagnostic()

  /**
   * @type {FSDiagnostic.DescriptorsDiagnostic}
   */
  descriptors = new FSDiagnostic.DescriptorsDiagnostic()
}

/**
 * A container for various timers diagnostics.
 */
export class TimersDiagnostic extends Diagnostic {
  /**
   * A container for core timeout timer diagnostics.
   */
  static TimeoutDiagnostic = class TimeoutDiagnostic extends Diagnostic {}

  /**
   * A container for core interval timer diagnostics.
   */
  static IntervalDiagnostic = class IntervalDiagnostic extends Diagnostic {}

  /**
   * A container for core immediate timer diagnostics.
   */
  static ImmediateDiagnostic = class ImmediateDiagnostic extends Diagnostic {}

  /**
   * @type {TimersDiagnostic.TimeoutDiagnostic}
   */
  timeout = new TimersDiagnostic.TimeoutDiagnostic()

  /**
   * @type {TimersDiagnostic.IntervalDiagnostic}
   */
  interval = new TimersDiagnostic.IntervalDiagnostic()

  /**
   * @type {TimersDiagnostic.ImmediateDiagnostic}
   */
  immediate = new TimersDiagnostic.ImmediateDiagnostic()
}

/**
 * A container for UDP diagnostics.
 */
export class UDPDiagnostic extends Diagnostic {}

/**
 * A container for various queried runtime diagnostics.
 */
export class QueryDiagnostic {
  posts = new PostsDiagnostic()
  childProcess = new ChildProcessDiagnostic()
  ai = new AIDiagnostic()
  fs = new FSDiagnostic()
  timers = new TimersDiagnostic()
  udp = new UDPDiagnostic()
  uv = new UVDiagnostic()
}

/**
 * Queries runtime diagnostics.
 * @return {Promise<QueryDiagnostic>}
 */
export async function query (type) {
  const result = await ipc.request('diagnostics.query')

  if (result.err) {
    throw result.err
  }

  const query = Object.assign(new QueryDiagnostic(), result.data)

  if (typeof globalThis.__global_ipc_extension_handler === 'function') {
    const result = await ipc.request('diagnostics.query', {}, {
      useExtensionIPCIfAvailable: true
    })

    if (result.data) {
      extend(query, Object.assign(new QueryDiagnostic(), result.data))
    }

    function extend (left, right) {
      for (const key in right) {
        if (Array.isArray(left[key]) && Array.isArray(right[key])) {
          left[key].push(...right[key])
        } else if (left[key] && typeof left[key] === 'object') {
          if (right[key] && typeof right[key] === 'object') {
            extend(left[key], right[key])
          }
        } else if (typeof left[key] === 'number' && typeof right[key] === 'number') {
          left[key] += right[key]
        } else {
          left[key] = right[key]
        }
      }
    }
  }

  if (typeof type === 'string') {
    return type
      .trim()
      .split(/\.|\[|\]/g)
      .map((key) => key.trim())
      .filter((key) => key.length > 0)
      .reduce((q, k) => q ? q[k] : null, query)
  }

  return query
}

export default {
  query
}
