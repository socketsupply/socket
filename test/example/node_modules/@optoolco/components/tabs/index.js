const Tonic = require('@optoolco/tonic')

const CustomEvent = window.CustomEvent

class TonicTabs extends Tonic {
  constructor (o) {
    super(o)

    this._setVisibilitySynchronously = false
    this.panels = {}
  }

  static stylesheet () {
    return `
      tonic-tabs .tonic--tab {
        -webkit-appearance: none;
        user-select: none;
      }

      tonic-tab a:focus {
        outline: none;
        background-color: var(--tonic-input-background);
      }
    `
  }

  get value () {
    const currentTab = this.querySelector('[aria-selected="true"]')
    if (currentTab) return currentTab.parentNode.id
  }

  set selected (value) {
    const tab = document.getElementById(value)

    if (tab) {
      this.setVisibility(tab.id, tab.getAttribute('for'))
    }
  }

  willConnect () {
    this.panels = this.panels || {}
  }

  async setVisibility (id, forAttr) {
    const renderAll = this.props.renderAll === 'true'
    const detatchOnHide = this.props.detatchOnHide
    const tabs = this.querySelectorAll('tonic-tab')

    for (const tab of tabs) {
      const control = tab.getAttribute('for')
      const anchor = tab.querySelector('a')

      if (!control) {
        throw new Error(`No "for" attribute found for tab id "${tab.id}".`)
      }

      const panelStore = this.panels[control]

      let panel = document.getElementById(control)
      if (!panel && panelStore) {
        panel = panelStore.node
      }

      if (!panel) {
        if (this._setVisibilitySynchronously) {
          return setTimeout(() => this.setVisibility(id, forAttr))
        }

        throw new Error(`No tonic-panel found that matches the id (${control})`)
      }

      if (tab.id === id || control === forAttr) {
        panel.removeAttribute('hidden')

        if (tab.id === id) {
          anchor.setAttribute('aria-selected', 'true')
        } else {
          anchor.setAttribute('aria-selected', 'false')
        }

        if (!panel.visible) {
          panel.visible = true
          if (panel.parentElement && panel.reRender && detatchOnHide) {
            await panel.reRender()
          }
        }

        if (!panel.parentElement && detatchOnHide) {
          panelStore.parent.appendChild(panel)
          if (
            panel.preventRenderOnReconnect && panel.reRender &&
            panel.children.length === 0
          ) {
            await panel.reRender()
          }
        }

        this.state.selected = id
        if (!this._setVisibilitySynchronously) {
          this.dispatchEvent(new CustomEvent(
            'tabvisible', { detail: { id }, bubbles: true }
          ))
        }
      } else {
        if (!panel.visible && renderAll && detatchOnHide) {
          panel.visible = true
          if (panel.parentElement && panel.reRender) {
            await panel.reRender()
          }
        }

        panel.setAttribute('hidden', '')
        if (detatchOnHide && panel.parentElement && !renderAll) {
          this.panels[panel.id] = {
            parent: panel.parentElement,
            node: panel
          }
          panel.remove()
        }

        if (anchor) {
          anchor.setAttribute('aria-selected', 'false')
        }

        if (!this._setVisibilitySynchronously) {
          this.dispatchEvent(new CustomEvent(
            'tabhidden', { detail: { id }, bubbles: true }
          ))
        }
      }
    }
  }

  click (e) {
    const tab = Tonic.match(e.target, '.tonic--tab')
    if (!tab) return

    e.preventDefault()

    this.setVisibility(tab.parentNode.id, tab.getAttribute('for'))
  }

  keydown (e) {
    const triggers = [...this.querySelectorAll('.tonic--tab')]

    switch (e.code) {
      case 'ArrowLeft':
      case 'ArrowRight': {
        const index = triggers.indexOf(e.target)
        const direction = (e.code === 'ArrowLeft') ? -1 : 1
        const length = triggers.length
        const newIndex = (index + length + direction) % length

        triggers[newIndex].focus()
        e.preventDefault()
        break
      }
      case 'Space': {
        const isActive = Tonic.match(e.target, '.tonic--tab:focus')
        if (!isActive) return

        e.preventDefault()

        const id = isActive.parentNode.getAttribute('id')
        this.setVisibility(id, isActive.getAttribute('for'))
        break
      }
    }
  }

  connected () {
    const id = this.state.selected || this.props.selected
    if (!id) {
      throw new Error('missing selected property.')
    }

    this._setVisibilitySynchronously = true
    this.setVisibility(id)
    this._setVisibilitySynchronously = false
  }

  render () {
    this.setAttribute('role', 'tablist')
    return this.html`${this.childNodes}`
  }
}

class TonicTabPanel extends Tonic {
  static stylesheet () {
    return `
      tonic-tab-panel {
        display: block;
      }

      tonic-tab-panel[hidden] {
        display: none;
      }
    `
  }

  constructor (o) {
    super(o)

    this.visible = this.visible || false
    this.__originalChildren = this.nodes

    if (!this.visible) {
      this.setAttribute('hidden', '')
    }
    this.setAttribute('role', 'tabpanel')
  }

  connected () {
    const tab = document.querySelector(
      `.tonic--tab[for="${this.props.id}"]`
    )
    if (tab) {
      const tabid = tab.getAttribute('id')
      this.setAttribute('aria-labelledby', tabid)
    }
  }

  disconnected () {
    if (this.props.detatch) {
      this.preventRenderOnReconnect = true
    }
  }

  render () {
    if (!this.visible && this.props.detatch) {
      return ''
    }

    if (this.props.detatch) {
      return this.html`${this.__originalChildren}`
    } else {
      return this.html`${this.childNodes}`
    }
  }
}

class TonicTab extends Tonic {
  render () {
    const ariaControls = this.props.for

    return this.html`
      <a
        id="${this.id}-anchor"
        for="${this.props.for}"
        class="tonic--tab"
        href="#"
        role="tab"
        aria-controls="${ariaControls}"
        aria-selected="false"
      >
        ${this.childNodes}
      </a>
    `
  }
}

module.exports = {
  TonicTabs,
  TonicTab,
  TonicTabPanel
}
