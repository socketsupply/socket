const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="profile-image">
  <tonic-sprite></tonic-sprite>
  <h2>Profile Image</h2>

  <!-- Profile Image Default -->
  <div class="test-container">
    <span>Default Profile Image</span>
    <tonic-profile-image
      id="profile-image-default">
    </tonic-profile-image>
  </div>

  <!-- Profile with id -->
  <div class="test-container">
    <span>id="profile-image-id"</span>
    <tonic-profile-image
      id="profile-image-id">
    </tonic-profile-image>
  </div>

  <!-- Profile with name -->
  <div class="test-container">
    <span>name="profile-name"</span>
    <tonic-profile-image
      id="profile-image-name"
      name="profile-name">
    </tonic-profile-image>
  </div>

  <!-- Profile with src -->
  <div class="test-container">
    <span>src="/sampleprofile.jpg"</span>
    <tonic-profile-image
      id="profile-image-src"
      src="/sampleprofile.jpg"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile with size -->
  <div class="test-container">
    <span>size="100px"</span>
    <tonic-profile-image
      id="profile-image-size"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile with radius -->
  <div class="test-container">
    <span>radius="50%"</span>
    <tonic-profile-image
      id="profile-image-radius"
      radius="50%"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile with border -->
  <div class="test-container">
    <span>border="3px solid black"</span>
    <tonic-profile-image
      id="profile-image-border"
      border="3px solid black"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile editable -->
  <div class="test-container">
    <span>editable="true"</span>
    <tonic-profile-image
      id="profile-image-editable"
      editable="true"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile not editable -->
  <div class="test-container">
    <span>editable="false"</span>
    <tonic-profile-image
      id="profile-image-editable-false"
      editable="false"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile theme light -->
  <div class="test-container">
    <span>theme="light"</span>
    <tonic-profile-image
      id="profile-image-theme-light"
      theme="light"
      size="100px">
    </tonic-profile-image>
  </div>

  <!-- Profile theme dark -->
  <div class="test-container dark">
    <span>theme="dark"</span>
    <tonic-profile-image
      id="profile-image-theme-dark"
      theme="dark"
      size="100px">
    </tonic-profile-image>
  </div>

</section>
`)

// TODO: write tests for profile-image
tape('test a profile image', t => {
  const img = qs('#profile-image-default')
  const imgDiv = qs('.tonic--image', img)
  const input = qs('input', img)
  const overlay = qs('.tonic--overlay', img)

  t.ok(img)
  t.ok(imgDiv)
  t.ok(input)
  t.ok(overlay)

  const styles = window.getComputedStyle(imgDiv)
  const styles2 = window.getComputedStyle(overlay)
  t.equal(styles.position, 'absolute')
  t.equal(styles2.position, 'absolute')

  t.end()
})
