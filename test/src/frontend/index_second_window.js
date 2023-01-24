import runtime from '../../../runtime.js'

runtime.send({ event: `secondary window ${runtime.currentWindow} loaded`, window: 0 })

document.querySelector('body > h1').textContent += ` ${runtime.currentWindow}`

window.addEventListener('character', e => {
  runtime.send({ event: `message from secondary window ${runtime.currentWindow}`, value: e.detail, window: 0 })
})
