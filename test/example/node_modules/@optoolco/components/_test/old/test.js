const Tonic = require('@optoolco/tonic')
const components = require('../..')

require('./tape.js')

components(Tonic)

function ready () {
  require('../../router/test')
  require('../../panel/test')
  require('../../dialog/test')
  require('../../tabs/test')
  require('../../windowed/test')
  require('../../tooltip/test')
  require('../../popover/test')
  require('../../badge/test')
  require('../../button/test')
  require('../../chart/test')
  require('../../checkbox/test')
  require('../../icon/test')
  require('../../input/test')
  require('../../progress-bar/test')
  require('../../profile-image/test')
  require('../../range/test')
  require('../../select/test')
  require('../../textarea/test')
  require('../../toaster/test')
  require('../../toaster-inline/test')
  require('../../toggle/test')

  document.addEventListener('keydown', e => {
    if (e.keyCode === 9) {
      document.body.classList.add('show-focus')
    }
  })

  document.addEventListener('click', e => {
    document.body.classList.remove('show-focus')
  })
}

document.addEventListener('DOMContentLoaded', ready)
