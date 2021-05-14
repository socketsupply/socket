const tape = require('@pre-bundled/tape')
const stream = tape.createStream({ objectMode: true })

const inc = id => {
  const el = document.getElementById(id)
  const count = el.querySelector('.value')

  return () => {
    const val = count.textContent.trim()
    count.textContent = parseInt(val, 10) + 1
  }
}

const incPassing = inc('passing')
const incTotal = inc('total')

const output = document.querySelector('#test-output')

let count = 0
let passed = 0

stream.on('data', data => {
  if (typeof data === 'string') {
    output.innerHTML += `<span class="comment">#${data}</span>`
  }

  if (data.type === 'test') {
    output.innerHTML += `<span class="title"># ${data.name}</span>\n`
    return
  }

  if (data.type === 'assert') {
    ++count
    incTotal()

    const status = data.ok ? 'OK' : 'FAIL'
    output.innerHTML += `<span class="result ${status}">${status} ${data.id} ${data.name}</span>`

    if (!data.ok) {
      console.error(data)
      return
    }

    ++passed
    incPassing()
    return
  }

  if (data.type === 'end') {
    const ok = passed === count ? 'OK' : 'FAIL'

    const status = document.getElementById('status')
    const value = status.querySelector('.value')

    if (!status.classList.contains('fail')) {
      value.textContent = ok
    }

    if (!ok) {
      status.classList.add('fail')
    }

    output.innerHTML += `\n<span class="${ok}"># ${ok ? 'ok' : 'not ok'}</span>`
  }
})

module.exports = tape
