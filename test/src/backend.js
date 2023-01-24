import backend from '../../backend.js'
import { test } from '@socketsupply/tapzero'

if (window.__args.os !== 'android' && window.__args.os !== 'ios') {
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
    t.deepEqual(backendSendData, { character: { firstname: 'Morty', secondname: 'Smith' } }, 'can send events with data')
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

  test('backend.sendToProcess()', async (t) => {
    const sendResult = await backend.sendToProcess({ firstname: 'Morty', secondname: 'Smith' })
    // TODO: what is the correct result?
    t.ok(sendResult.err == null, 'returns correct result')
    const character = await new Promise(resolve => window.addEventListener('character.backend', ({ detail }) => resolve(detail), { once: true }))
    t.deepEqual(character, { character: { firstname: 'Summer', secondname: 'Smith' } }, 'send data to process')
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
