import { spawn, exec } from 'socket:child_process'
import process from 'socket:process'
import test from 'socket:test'
import os from 'socket:os'

test('child_process.spawn(command[,args[,options]])', async (t) => {
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

test('child_process.exec(command[,options],callback)', async (t) => {
  const pending = []

  pending.push(new Promise((resolve, reject) => {
    exec('ls -la', (err, stdout, stderr) => {
      if (err) {
        return reject(err)
      }

      t.ok(stdout, 'there is stdout')
      resolve()
    })
  }))

  pending.push(new Promise((resolve, reject) => {
    exec('ls /not/a/directory', (err, stdout, stderr) => {
      if (err) {
        return reject(err)
      }

      t.ok(!stdout, 'there is no stdout')
      t.ok(stderr, 'there is no stdout')
      resolve()
    })
  }))

  await Promise.all(pending)
})

test('await child_process.exec(command)', async (t) => {
  const { stdout } = await exec('ls -la')
  t.ok(stdout && stdout.length, 'stdout from await exec() has output')
})
