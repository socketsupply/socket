const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

document.body.appendChild(html`
<section id="textarea">
  <h2>Textarea</h2>

  <div class="test-container">
    <span>Default Textarea</span>
    <tonic-textarea id="text-area-1"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>With Content</span>
    <tonic-textarea id="text-area-2">It was the best of times, it was the worst of times, it was the age of wisdom, it was the age of foolishness, it was the epoch of belief, it was the epoch of incredulity, it was the season of Light, it was the season of Darkness, it was the spring of hope, it was the winter of despair, we had everything before us, we had nothing before us, we were all going direct to Heaven, we were all going direct the other way—in short, the period was so far like the present period, that some of its noisiest authorities insisted on its being received, for good or for evil, in the superlative degree of comparison only.</tonic-textarea>
  </div>

  <div class="test-container">
    <span>id="textarea-id"</span>
    <tonic-textarea id="textarea-id"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>name="textarea-name"</span>
    <tonic-textarea id="text-area-3" name="textarea-name"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>placeholder="This is a placeholder"</span>
    <tonic-textarea id="text-area-4" placeholder="This is a placeholder"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>resize="none"</span>
    <tonic-textarea id="text-area-5" resize="none"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>spellcheck="true"</span>
    <tonic-textarea id="text-area-6" spellcheck="true">fdsfds</tonic-textarea>
  </div>

  <div class="test-container">
    <span>spellcheck="false"</span>
    <tonic-textarea id="text-area-7" spellcheck="false">fdsfds</tonic-textarea>
  </div>

  <div class="test-container">
    <span>disabled="true"</span>
    <tonic-textarea id="text-area-8" disabled="true"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>disabled="false"</span>
    <tonic-textarea id="text-area-9" disabled="false"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>required="true"</span>
    <tonic-textarea id="text-area-10" required="true"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>required="false"</span>
    <tonic-textarea id="text-area-11" required="false"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>readonly="true"</span>
    <tonic-textarea id="text-area-12" readonly="true"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>readonly="false"</span>
    <tonic-textarea id="text-area-13" readonly="false"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>autofocus="true"</span>
    <tonic-textarea id="text-area-14" autofocus="true"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>autofocus="false"</span>
    <tonic-textarea id="text-area-15" autofocus="false"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>rows="10"</span>
    <tonic-textarea id="text-area-16" rows="10"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>cols="10"</span>
    <tonic-textarea id="text-area-17" cols="10"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>minlength="10"</span>
    <tonic-textarea id="text-area-18" minlength="10"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>maxlength="100"</span>
    <tonic-textarea id="text-area-19" maxlength="100"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>width="100%"</span>
    <tonic-textarea id="text-area-20" width="100%"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>width="200px"</span>
    <tonic-textarea id="text-area-21" width="100%"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>height="300px"</span>
    <tonic-textarea id="text-area-22" height="300px"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>radius="10px"</span>
    <tonic-textarea id="text-area-23" radius="10px"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>theme="light"</span>
    <tonic-textarea id="text-area-24" theme="light"></tonic-textarea>
  </div>

  <div class="test-container flex-half dark">
    <span>theme="dark"</span>
    <tonic-textarea id="text-area-25" theme="dark"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>label="Foo Bar Bazz"</span>
    <tonic-textarea id="text-area-26" label="Foo Bar Bazz"></tonic-textarea>
  </div>

  <div class="test-container">
    <span>autofocus="true"</span>
    <tonic-textarea id="text-area-27" autofocus="true"></tonic-textarea>
  </div>

</section>
`)

// TODO: write tests
