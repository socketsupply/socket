import { createRequire } from 'socket:module'

const require = createRequire(import.meta.url)
require('./commonjs/')
require('./commonjs/scope')
require('./commonjs/globals')
require('./commonjs/builtins')
require('./commonjs/resolvers')
require('./commonjs/node-modules')
require('./commonjs/special-top-level-await')
