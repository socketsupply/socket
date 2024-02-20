import test from 'socket:test'
import { spawn } from 'socket:child_process'

test('basic spawn', async t => {
  const command = 'ls'
  const args = ['-la']
  const options = {}

  let hasDir = false

  await new Promise((resolve, reject) => {
    const c = spawn(command, args, options)

    c.stdout.on('data', data => {
      if (Buffer.from(data).toString().includes('child_process')) {
        hasDir = true
      }
    })

    c.on('exit', resolve)
    c.on('error', reject)
  })

  t.ok(hasDir, 'the ls command ran and discovered the child_process directory')
})
