import './test-context.js' // this should be first
import os from 'socket:os'

// @TODO(jwerle): tapzero needs a `t.plan()` so we know exactly how to
// expect the total pass count

import './ipc.js'
import './os.js'
import './process.js'
import './path.js'
import './dgram.js'
import './dns.js'
import './backend.js'
import './crypto.js'
import './util.js'

// TODO(@jwerle): FIXME
if (os.platform() !== 'win32') {
  await import('./runtime.js')
  await import('./fs.js')
}
