import { createRequire } from 'socket:module'

const require = createRequire(import.meta.url)
require('./commonjs')
