import os from 'node:os'

export async function load () {
  const platform = os.platform()
  const arch = os.arch()
  const info = await import(`@socketsupply/socket-${platform}-${arch}`)
  return info?.default ?? info ?? null
}
