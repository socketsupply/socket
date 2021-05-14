const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="toaster">
  <h2>Toaster</h2>

  <tonic-toaster position="center"></tonic-toaster>
  <tonic-toaster position="left"></tonic-toaster>
  <tonic-toaster position="right"></tonic-toaster>
  <tonic-toaster theme="light"></tonic-toaster>
  <tonic-toaster theme="dark"></tonic-toaster>

  <div id="toaster-2" class="test-container">
    <span>Toaster w/ Type Success</span>
    <tonic-button id="tonic-toaster-type-success">
      Success
    </tonic-button>
  </div>

  <div id="toaster-3" class="test-container">
    <span>Toaster w/ Type warning</span>
    <tonic-button id="tonic-toaster-type-warning">
      Warning
    </tonic-button>
  </div>

  <div id="toaster-4" class="test-container">
    <span>Toaster w/ Type danger</span>
    <tonic-button id="tonic-toaster-type-danger">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ type info -->
  <div class="test-container">
    <span>Toaster w/ Type info</span>
    <tonic-button id="tonic-toaster-type-info">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ title -->
  <div class="test-container">
    <span>Toaster w/ Title</span>
    <tonic-button id="tonic-toaster-title">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ message -->
  <div class="test-container">
    <span>Toaster w/ Message</span>
    <tonic-button id="tonic-toaster-message">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ title and message -->
  <div class="test-container">
    <span>Toaster w/ Title & Message</span>
    <tonic-button id="tonic-toaster-title-message">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ title and message and type -->
  <div class="test-container">
    <span>Toaster w/ Type, Title & Message</span>
    <tonic-button id="tonic-toaster-type-title-message">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ dismiss -->
  <div class="test-container">
    <span>Toaster w/ dismiss</span>
    <tonic-button id="tonic-toaster-dismiss">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/out dismiss -->
  <div class="test-container">
    <span>Toaster w/out dismiss</span>
    <tonic-button id="tonic-toaster-dismiss-false">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ dismiss w/ duration -->
  <div class="test-container">
    <span>Toaster w/ dismiss w/ duration</span>
    <tonic-button id="tonic-toaster-dismiss-duration">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/out dismiss w/ duration -->
  <div class="test-container">
    <span>Toaster w/out dismiss w/ duration</span>
    <tonic-button id="tonic-toaster-dismiss-false-duration">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ left position -->
  <div class="test-container">
    <span>Toaster w/ left position</span>
    <tonic-button id="tonic-toaster-position-left">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ right position -->
  <div class="test-container">
    <span>Toaster w/ right position</span>
    <tonic-button id="tonic-toaster-position-right">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ theme light -->
  <div class="test-container">
    <span>Toaster w/ theme light</span>
    <tonic-button id="tonic-toaster-theme-light" theme="light">
      Toast
    </tonic-button>
  </div>

  <!-- Toaster w/ theme dark -->
  <div class="test-container flex-half dark">
    <span>Toaster w/ theme dark</span>
    <tonic-button id="tonic-toaster-theme-dark" theme="dark">
      Toast
    </tonic-button>
  </div>

</section>
`)

const notification = qs('tonic-toaster[position="center"]')
const sleep = n => new Promise(resolve => setTimeout(resolve, n))

tape('{{toaster}} is created and destroyed', async t => {
  notification.create({
    message: 'You have been notified.'
  })

  await sleep(512)

  const toaster = qs('.tonic--notification', notification)
  const toasterMain = qs('.tonic--main', toaster)
  const toasterMessage = qs('.tonic--message', toasterMain)
  const toasterTitle = qs('.tonic--title', toasterMain)
  const dismiss = toaster.classList.contains('tonic--close')
  const closeIcon = qs('.tonic--close', toaster)

  t.plan(6)

  t.ok(toaster, 'Toaster was created')
  t.ok(toasterMain, 'Toaster main div was created')
  t.ok(toasterMessage, 'Toaster message div was created')
  t.ok(toasterTitle, 'Toaster title div was created')

  t.equal(!dismiss, !closeIcon, 'Only if toaster has dismiss class, is close icon also created')

  notification.destroy(toaster)

  await sleep(512)

  const toasterB = qs('.tonic--notification', notification)
  t.ok(!toasterB, 'Toaster was destroyed')

  t.end()
})

tape('{{toaster}} with dismiss false is created without close icon', async t => {
  notification.create({
    message: 'I will stay open',
    dismiss: 'false'
  })

  await sleep(512)

  const toaster = qs('.tonic--notification', notification)
  const dismiss = toaster.classList.contains('tonic--close')
  const closeIcon = qs('.tonic--close', toaster)

  t.equal(!dismiss, !closeIcon, 'Only if toaster has dismiss class, is close icon also created')

  notification.destroy(toaster)
  t.end()
})

tape('{{toaster}} with type success is created', async t => {
  notification.create({
    type: 'success',
    message: 'Success!'
  })

  await sleep(512)

  const toaster = qs('.tonic--notification', notification)
  const toasterMessage = qs('.tonic--message', toaster)

  const alert = toaster.classList.contains('tonic--alert')
  const alertIcon = qs('.tonic--icon', toaster)

  t.ok(toaster, 'Toaster was created')
  t.equal(toasterMessage.textContent, 'Success!', 'Toaster textContent matches message')
  t.equal(!alert, !alertIcon, 'If toaster does not have alert class, alert icon is not created')

  notification.destroy(toaster)
  t.end()
})

tape('{{toaster}} is created and destroyed after duration', async t => {
  notification.create({
    message: 'Short and sweet',
    duration: 512
  })

  await sleep(128)

  const toaster = qs('.tonic--notification', notification)
  t.ok(toaster, 'Toaster was created')

  await sleep(1024)

  const toasterB = qs('.tonic--notification', notification)
  t.ok(!toasterB, 'Toaster was destroyed')

  t.end()
})

tape('{{toaster}} is created on the left', async t => {
  const notificationLeft = qs('tonic-toaster[position="left"]')
  const wrapper = qs('.tonic--left', notificationLeft)

  notificationLeft.create({
    message: 'Left toaster',
    duration: 3e3
  })

  await sleep(128)

  const toaster = qs('.tonic--notification', wrapper)

  t.ok(wrapper, 'Wrapper was created with the tonic--left class')
  t.ok(toaster, 'Toaster was created')

  t.end()
})

tape('{{toaster}} is created on the right', async t => {
  const notificationRight = qs('tonic-toaster[position="right"]')
  const wrapper = qs('.tonic--right', notificationRight)

  notificationRight.create({
    message: 'Right toaster',
    duration: 3e3
  })

  await sleep(128)

  const toaster = qs('.tonic--notification', wrapper)

  t.ok(wrapper, 'Wrapper was created with the tonic--right class')
  t.ok(toaster, 'Toaster was created')

  t.end()
})
