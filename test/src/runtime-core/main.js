import extension from 'socket:extension'
import process from 'socket:process'

try {
  await extension.load('runtime-core-tests')
  process.exit(0)
} catch (err) {
  console.error(err.message || err)
  process.exit(1)
}
