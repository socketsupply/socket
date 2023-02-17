import application from 'socket:application'

const currentWindow = await application.getCurrentWindow()
currentWindow.send({ event: 'secondary window loaded', window: 0 })

document.querySelector('body > h1').textContent += ` ${currentWindow.index}`

window.addEventListener('character', e => {
  currentWindow.send({ event: 'message from secondary window', value: e.detail, window: 0 })
})
