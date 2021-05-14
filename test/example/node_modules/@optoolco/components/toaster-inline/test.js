
const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="toaster-inline">
  <h2>ToasterInline</h2>

  <!-- Default inline toaster -->
  <div id="inline-toaster-1" class="test-container">
    <span>Default Inline Toaster</span>
    <tonic-toaster-inline
      id="tonic-toasterinline-default">
      Message
    </tonic-toaster-inline>
  </div>

  <!-- Toaster inline displayed initially -->
  <div id="inline-toaster-2" class="test-container">
    <span>Toaster inline displayed initially</span>
    <div>
      <tonic-toaster-inline
        display="true"
        id="tonic-toasterinline-initial">
        Displayed initially.</tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ title -->
  <div id="inline-toaster-5" class="test-container">
    <span>Toaster inline w/ title</span>
    <div>
      <tonic-toaster-inline
        id="tonic-toasterinline-title">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ inline HTML title -->
  <div id="inline-toaster-6" class="test-container">
    <span>Toaster inline w/ inline HTML title</span>
    <div>
      <tonic-toaster-inline
        title="I am a title!"
        id="tonic-toasterinline-inline-title"></tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ message -->
  <div id="inline-toaster-7" class="test-container">
    <span>Toaster inline w/ message</span>
    <div>
      <tonic-toaster-inline
        id="tonic-toasterinline-message">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ message (initial) -->
  <div id="inline-toaster-8" class="test-container">
    <span>Toaster inline w/ message (initial)</span>
    <div>
      <tonic-toaster-inline
        display="true"
        message="I am a message!"
        id="tonic-toasterinline-message-2">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ title & message -->
  <div id="inline-toaster-9" class="test-container">
    <span>Toaster inline w/ title & message</span>
    <div>
      <tonic-toaster-inline
        id="tonic-toasterinline-title-message">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ innerHTML message (must be initial)-->
  <div id="inline-toaster-10" class="test-container">
    <span>Toaster inline w/ innerHTML message (must be initial)</span>
    <div>
      <tonic-toaster-inline
        display="true"
        id="tonic-toasterinline-innerHTML-message">
        This is your message.
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type success -->
  <div id="inline-toaster-11" class="test-container">
    <span>Toaster inline w/ type success</span>
    <div>
      <tonic-toaster-inline
        type="success"
        id="tonic-toasterinline-type-success">
        It's a success
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type warning -->
  <div id="inline-toaster-12" class="test-container">
    <span>Toaster inline w/ type warning</span>
    <div>
      <tonic-toaster-inline
        type="warning"
        id="tonic-toasterinline-type-warning">
        You've been warned.
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type danger -->
  <div id="inline-toaster-13" class="test-container">
    <span>Toaster inline w/ type danger</span>
    <div>
      <tonic-toaster-inline
        type="danger"
        id="tonic-toasterinline-type-danger">
        Watch out!
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type info -->
  <div id="inline-toaster-14" class="test-container">
    <span>Toaster inline w/ type info</span>
    <div>
      <tonic-toaster-inline
        type="info"
        id="tonic-toasterinline-type-info">
        For your information
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type & title -->
  <div id="inline-toaster-15" class="test-container">
    <span>Toaster inline w/ type & title</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-type-title">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type & message -->
  <div id="inline-toaster-16" class="test-container">
    <span>Toaster inline w/ type & message</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-type-message">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ type & title & message -->
  <div id="inline-toaster-17" class="test-container">
    <span>Toaster inline w/ type & title & message</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-type-title-message">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ duration -->
  <div id="inline-toaster-18" class="test-container">
    <span>Toaster inline w/ duration</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-duration">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/out duration -->
  <div id="inline-toaster-19" class="test-container">
    <span>Toaster inline w/out duration</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-no-duration">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/ dismiss w/ duration -->
  <div id="inline-toaster-20" class="test-container">
    <span>Toaster inline w/ dismiss w/ duration</span>
    <div>
      <tonic-toaster-inline
        id="tonic-toasterinline-dismiss-duration"
        dismiss="true"
        message="Dismiss me or I will disappear!">
      </tonic-toaster-inline>
    </div>
  </div>

  <!-- Inline toaster w/out dismiss w/ duration -->
  <div id="inline-toaster-21" class="test-container">
    <span>Toaster inline w/out dismiss w/ duration</span>
    <div>
      <tonic-toaster-inline id="tonic-toasterinline-no-dismiss-duration">
        Message
      </tonic-toaster-inline>
    </div>
  </div>

</section>
`)
