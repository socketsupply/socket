/* global MutationObserver */
import ipc from './ipc.js'

import * as exports from './polyfills.js'

export function applyPolyfills (window) {
  Object.defineProperties(window, Object.getOwnPropertyDescriptors({
    resizeTo (w, h) {
      const index = window.__args.index
      const targetWindow = window.__args.index
      const isWidthInPercent = typeof w === 'string' && w.endsWith('%')
      const isHeightInPercent = typeof h === 'string' && h.endsWith('%')
      const width = isWidthInPercent ? Number(w.slice(0, -1)) : w
      const height = isHeightInPercent ? Number(h.slice(0, -1)) : h
      const o = new URLSearchParams({ index, targetWindow, width, height, isWidthInPercent, isHeightInPercent }).toString()
      ipc.postMessage(`ipc://window.setSize?${o}`)
      // TODO: update the window with index window.__args.index
    }
  }))

  // create <title> tag in document if it doesn't exist
  window.document.title ||= ''
  // initial value
  window.addEventListener('DOMContentLoaded', async () => {
    const title = window.document.title
    if (title.length !== 0) {
      const index = window.__args.index
      const o = new URLSearchParams({ value: title, index }).toString()
      ipc.postMessage(`ipc://window.setTitle?${o}`)
    }
  })

  //
  // window.document is uncofigurable property so we need to use MutationObserver here
  //
  const observer = new MutationObserver((mutationList) => {
    for (const mutation of mutationList) {
      if (mutation.type === 'childList') {
        const index = window.__args.index
        const title = mutation.addedNodes[0].textContent
        const o = new URLSearchParams({ value: title, index }).toString()
        ipc.postMessage(`ipc://window.setTitle?${o}`)
      }
    }
  })

  const titleElement = document.querySelector('head > title')
  if (titleElement) {
    observer.observe(titleElement, { childList: true })
  }
}

export default exports
