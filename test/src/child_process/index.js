import { spawn } from 'socket:child_process'
import process from 'socket:process'
import test from 'socket:test'
import os from 'socket:os'

test('basic spawn', async t => {
  const command = 'ls'
  const args = ['-la']
  const options = {}

  let hasDir = false

  const pending = []
  const child = spawn(command, args, options)

  if (/linux|darwin/i.test(os.platform())) {
    pending.push(new Promise((resolve, reject) => {
      const timeout = setTimeout(
        () => reject(new Error('Timed out aiting for SIGCHLD signal')),
        1000
      )

      process.once('SIGCHLD', (signal, code, message) => {
        resolve()
        clearTimeout(timeout)
      })
    }))
  }

  pending.push(new Promise((resolve, reject) => {
    child.stdout.on('data', data => {
      if (Buffer.from(data).toString().includes('child_process')) {
        hasDir = true
      }
    })

    child.on('exit', resolve)
    child.on('error', reject)
  }))

  await Promise.all(pending)

  t.ok(hasDir, 'the ls command ran and discovered the child_process directory')
})
