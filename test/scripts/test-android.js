import { execSync as exec } from 'node:child_process'
import path from 'node:path'

const { ANDROID_HOME, SSC_ANDROID_CI } = process.env
const dirname = path.dirname(import.meta.url.replace('file://', ''))
const root = path.dirname(dirname)
const adb = `${ANDROID_HOME}/platform-tools/adb`
const id = 'co.socketsupply.socket.tests'

try {
  exec(`${adb} uninstall ${id}`, { stdio: 'inherit' })
} catch {
}

if (SSC_ANDROID_CI) {
  exec('ssc build -r -o --test=./index.js --headless --platform=android --env=CI --env=SSC_ANDROID_CI', {
    stdio: 'inherit'
  })
} else {
  exec('ssc build -r -o --test=./index.js --prod --headless --platform=android --env SOCKET_DEBUG_IPC', {
    stdio: 'inherit'
  })
}

try {
  exec(`${adb} shell rm -rf /data/local/tmp/ssc-socket-test-fixtures`, {
    stdio: 'inherit'
  })
} catch {
}

exec(`${adb} push ${path.join(root, 'fixtures')} /data/local/tmp/ssc-socket-test-fixtures`, {
  stdio: 'inherit'
})

exec(`${process.env.SHELL || 'sh'} ${path.resolve(root, 'scripts', 'poll-adb-logcat.sh')}`, {
  stdio: 'inherit'
})
