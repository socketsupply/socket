import { test } from 'socket:test'
import backend from 'socket:backend'
import process from 'socket:process'
import application from 'socket:application'

if (!['android', 'ios'].includes(process.platform)) {
  test('backend.open()', async (t) => {
    const openResult = await backend.open()
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
  })

  test('backend', async (t) => {
    const successOpenPromise = Promise.race([
      new Promise(resolve => window.addEventListener('process-error', ({ detail }) => {
        if (detail) t.fail(`process-error event emitted: ${detail}`)
        resolve(false)
      }, { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 0))
    ])
    const backendReadyPromise = new Promise(resolve => window.addEventListener('backend:ready', () => resolve(true), { once: true }))
    const backenSendDataPromise = new Promise(resolve => window.addEventListener('character', ({ detail }) => resolve(detail), { once: true }))
    const [successOpen, backendReady, backendSendData] = await Promise.all([successOpenPromise, backendReadyPromise, backenSendDataPromise])
    t.ok(successOpen, 'does not emit a process-error event')
    t.ok(backendReady, 'can send events to window 0')
    t.deepEqual(backendSendData, { firstname: 'Morty', secondname: 'Smith' }, 'can send events with data')
  })

  test('backend.open() again', async (t) => {
    const openResult = await backend.open()
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
    const doesNotRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(false), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 256))
    ])
    t.ok(doesNotRestart, 'does not emit a backend:ready event')
  })

  test('backend.open({ force: true })', async (t) => {
    const openResult = await backend.open({ force: true })
    t.deepEqual(Object.keys(openResult.data).sort(), ['argv', 'cmd', 'path'], 'returns a result with the correct keys')
    const doesRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(true), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(false), 256))
    ])
    t.ok(doesRestart, 'emits a backend:ready event')
  })

  test('window.send to backend', async (t) => {
    const currentWindow = await application.getCurrentWindow()
    const value = { firstname: 'Rick', secondname: 'Sanchez' }
    await currentWindow.send({ event: 'character', value, backend: true })
    const result = await new Promise(resolve => window.addEventListener('character.backend', ({ detail }) => resolve(detail), { once: true }))
    t.deepEqual(result, value, 'send to backend and back succeeds')
  })

  test('backend.close()', async (t) => {
    const closeResult = await backend.close()
    t.ok(closeResult.err == null, 'returns correct result')
    const doesNotRestart = await Promise.race([
      new Promise(resolve => window.addEventListener('backend:ready', () => resolve(false), { once: true })),
      new Promise(resolve => setTimeout(() => resolve(true), 256))
    ])
    t.ok(doesNotRestart, 'does not emit a backend:ready event')
  })
}
