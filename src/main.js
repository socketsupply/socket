process.stdin.setEncoding('utf8')

process.stdin.on('data', d => {
  process.stdout.write(JSON.stringify({input: JSON.parse(d) }))
})

setInterval(() => {
  process.stdout.write(JSON.stringify({ output: String(Math.random()) }))
}, 16)
