import extension from 'socket:extension'
import process from 'socket:process'

const EXIT_TIMEOUT = 250

try {
  await extension.load('runtime-core-tests')
  setTimeout(() => process.exit(0), EXIT_TIMEOUT)
} catch (err) {
  if (!/failed to load/i.test(err?.message)) {
    console.error(err.message || err)
  }

  setTimeout(() => process.exit(1), EXIT_TIMEOUT)
}
