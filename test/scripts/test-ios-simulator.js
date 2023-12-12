import { rmSync as rm, cpSync as cp } from 'node:fs'
import { execSync, spawn } from 'node:child_process'
import path from 'node:path'

const dirname = path.dirname(import.meta.url.replace('file://', '').replace(/^\/[A-Za-z]:/, ''))
const root = path.dirname(dirname)
const child = spawn('ssc', ['build', '--headless', '--test=./index.js', '--platform=ios-simulator', '--prod', '-r', '-o', '--env', 'SOCKET_DEBUG_IPC'], {
  stdio: 'inherit'
})

const interval = setInterval(() => {
  let container = null
  if (child.exitCode !== null || child.killed) {
    console.warn('iOS Simulator exited before determining the container data directory')
    clearInterval(interval)
    return
  }

  try {
    container = execSync('xcrun simctl get_app_container booted co.socketsupply.socket.tests data')
    if (container) {
      container = container.toString().trim()
    }
  } catch (err) {
    if (err.status !== 2) {
      clearInterval(interval)
      throw err
    }
  }

  if (!container) {
    return // continue
  }

  const TMPDIR = path.join(container, 'tmp')

  try {
    rm(path.join(TMPDIR, 'ssc-socket-test-fixtures'), {
      recursive: true,
      force: true
    })
  } catch {}

  cp(path.join(root, 'fixtures'), path.join(TMPDIR, 'ssc-socket-test-fixtures'), {
    recursive: true
  })

  clearInterval(interval)
}, 200)
