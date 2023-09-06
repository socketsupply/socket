import { execSync as exec } from 'node:child_process'

exec('ssc build --headless --test=./index.js --platform=ios-simulator --prod -r -o', {
  stdio: 'inherit'
})
