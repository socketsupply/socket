import test from 'socket:test'

test('queueMicrotask', async (t) => {
  await new Promise((resolve) => {
    queueMicrotask(resolve)
  })

  t.pass('queueMicrotask calls callback')
})

test('queueMicrotask - sync error', async (t) => {
  await new Promise((resolve) => {
    globalThis.addEventListener('error', resolve)
    queueMicrotask(() => {
      throw Object.assign(new Error('error'), { ignore: true })
    })
  })

  t.pass('queueMicrotask bubbles sync error')
})

test('queueMicrotask - async error', async (t) => {
  await new Promise((resolve) => {
    globalThis.addEventListener('unhandledrejection', resolve)
    queueMicrotask(async () => {
      throw Object.assign(new Error('error'), { ignore: true })
    })
  })

  t.pass('queueMicrotask bubbles async error')
})
