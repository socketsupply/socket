import { test } from 'socket:test'
import mime from 'socket:mime'

test('mime.lookup', async (t) => {
  const results = await mime.lookup('html')
  t.ok(results)
  // TODO: introduce common test cases
})
