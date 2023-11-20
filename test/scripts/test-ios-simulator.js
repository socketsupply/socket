import { execSync as exec } from 'node:child_process'

exec('ssc build --headless --test=./index.js --platform=ios-simulator --prod -r -o --env SOCKET_DEBUG_IPC', {
  stdio: 'inherit'
})
