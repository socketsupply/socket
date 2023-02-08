/* global MutationObserver */
import console from './console.js'
import ipc from './ipc.js'

export function applyPolyfills (window) {
  Object.defineProperties(window, Object.getOwnPropertyDescriptors({
    resizeTo (width, height) {
      const index = window.__args.index
      const o = new URLSearchParams({ index, width, height }).toString()
      return ipc.postMessage(`ipc://window.setSize?${o}`)
    },

    // TODO(@heapwolf) the properties do not yet conform to the MDN spec
    async showOpenFilePicker (o) {
      console.warn('window.showOpenFilePicker may not conform to the standard')
      const { data } = await ipc.send('dialog', { type: 'open', ...o })
      return typeof data === 'string' ? data.split('\n') : []
    },

    // TODO(@heapwolf) the properties do not yet conform to the MDN spec
    async showSaveFilePicker (o) {
      console.warn('window.showSaveFilePicker may not conform to the standard')
      const { data } = await ipc.send('dialog', { type: 'save', ...o })
      return typeof data === 'string' ? data.split('\n') : []
    },

    // TODO(@heapwolf) the properties do not yet conform to the MDN spec
    async showDirectoryFilePicker (o) {
      console.warn('window.showDirectoryFilePicker may not conform to the standard')
      const { data } = await ipc.send('dialog', { allowDirs: true, ...o })
      return typeof data === 'string' ? data.split('\n') : []
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
