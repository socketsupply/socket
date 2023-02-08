import * as ipc from '../ipc.js'

export default ipc.createBinding('fs', {
  default: 'request',
  write: 'write'
})
