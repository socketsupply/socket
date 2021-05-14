'use strict'

window.promiseCounter = 0
const OldPromise = window.Promise
window.Promise = function () {
  window.promiseCounter++
  console.trace('a promise here')
  return new OldPromise(...arguments)
}

const Tonic = require('@optoolco/tonic')

const components = require('../../')
const { html } = require('../util')

require('./test-form')

components(Tonic)

document.body.appendChild(html`
  <div>
    <style>
    body {
      --tonic-body: 'Avenir-Light', sans-serif;
      --tonic-header: 'Avenir-Light', sans-serif;
      --tonic-subheader: 'Avenir-Medium', sans-serif;
      --tonic-monospace: 'IBM Plex Mono', monospace;
    }

    body, *[theme="light"] {
      --tonic-window: rgba(255, 255, 255, 1);
      --tonic-primary: rgba(54, 57, 61, 1);
      --tonic-disabled: rgba(152, 161, 175, 1);
      --tonic-secondary: rgba(232, 232, 228, 1);
      --tonic-medium: rgba(153, 157, 160, 1);
      --tonic-accent: rgba(240, 102, 83, 1);
      --tonic-button: rgba(54, 57, 61, 1);
      --tonic-border: rgba(232, 232, 228, 1);
      --tonic-background: rgba(247, 247, 245, 1);
      --tonic-error: rgba(240, 102, 83, 1);
      --tonic-notification: rgba(240, 102, 83, 1);
      --tonic-danger: rgba(240, 102, 83, 1);
      --tonic-success: rgba(133, 178, 116, 1);
      --tonic-warning: rgba(249, 169, 103, 1);
      --tonic-info: rgba(153, 157, 160, 1);
    }

    *[theme="dark"] {
      --tonic-window: rgba(45, 47, 49, 1);
      --tonic-primary: rgba(255, 255, 255, 1);
      --tonic-disabled: rgba(170, 170, 170, 1);
      --tonic-secondary: rgba(195, 195, 195, 1);
      --tonic-medium: rgba(153, 157, 160, 1);
      --tonic-accent: rgba(240, 102, 83, 1);
      --tonic-button: rgba(255, 255, 255, 1);
      --tonic-border: rgb(107, 107, 107);
      --tonic-background: rgba(60, 60, 60, 1);
      --tonic-error: rgba(240, 102, 83, 1);
      --tonic-notification: rgba(240, 102, 83, 1);
      --tonic-caution: rgba(240, 102, 83, 1);
      --tonic-success: rgba(133, 178, 116, 1);
      --tonic-warn: rgba(249, 169, 103, 1);
    }
    </style>
    <test-form id="test-form"></test-form>
    <custom-event-parent id="custom-event-parent"></custom-event-parent>
  </div>
`)
