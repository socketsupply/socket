const getNRandom = (n, arr) => {
  return arr.sort(() => Math.random() - 0.5).slice(0, n)
}

const getRandomPort = (localPort, testPort) => {
  const portCache = { [localPort]: true, [testPort]: true }
  const next = () => ~~(Math.random() * 0xffff) || 1

  return (ports = portCache, p) => {
    do ports[p = next()] = true; while (!ports[p] && p)
    return p
  }
}

export {
  getNRandom,
  getRandomPort
}
