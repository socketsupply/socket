import { ServiceWorker } from './service-worker/instance.js'
import { SharedWorker } from './internal/shared-worker.js'
import { Worker } from './worker_threads.js'

export { SharedWorker, ServiceWorker, Worker }
export default Worker
