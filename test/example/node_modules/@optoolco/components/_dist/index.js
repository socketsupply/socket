(function(f){if(typeof exports==="object"&&typeof module!=="undefined"){module.exports=f()}else if(typeof define==="function"&&define.amd){define([],f)}else{var g;if(typeof window!=="undefined"){g=window}else if(typeof global!=="undefined"){g=global}else if(typeof self!=="undefined"){g=self}else{g=this}g.components = f()}})(function(){var define,module,exports;return (function(){function r(e,n,t){function o(i,f){if(!n[i]){if(!e[i]){var c="function"==typeof require&&require;if(!f&&c)return c(i,!0);if(u)return u(i,!0);var a=new Error("Cannot find module '"+i+"'");throw a.code="MODULE_NOT_FOUND",a}var p=n[i]={exports:{}};e[i][0].call(p.exports,function(r){var n=e[i][1][r];return o(n||r)},p,p.exports,r,e,n,t)}return n[i].exports}for(var u="function"==typeof require&&require,i=0;i<t.length;i++)o(t[i]);return o}return r})()({1:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicAccordion extends Tonic {
  defaults () {
    return {
      multiple: false
    }
  }

  static stylesheet () {
    return `
      tonic-accordion {
        display: block;
        border: 1px solid var(--tonic-border, black);
        border-radius: 2px;
      }
    `
  }

  qs (s) {
    return this.querySelector(s)
  }

  qsa (s) {
    return [...this.querySelectorAll(s)]
  }

  setVisibility (id) {
    const trigger = document.getElementById(id)
    if (!trigger) return

    const allowMultiple = this.hasAttribute('data-allow-multiple')
    const isExpanded = trigger.getAttribute('aria-expanded') === 'true'

    if (!isExpanded && !allowMultiple) {
      const triggers = this.qsa('.tonic--accordion-header button')
      const panels = this.qsa('.tonic--accordion-panel')

      triggers.forEach(el => el.setAttribute('aria-expanded', 'false'))
      panels.forEach(el => el.setAttribute('hidden', ''))
    }

    const panelId = trigger.getAttribute('aria-controls') ||
      `tonic--accordion-panel-${trigger.id}`

    if (isExpanded) {
      trigger.setAttribute('aria-expanded', 'false')
      const currentPanel = document.getElementById(panelId)
      if (currentPanel) currentPanel.setAttribute('hidden', '')
      return
    }

    trigger.setAttribute('aria-expanded', 'true')
    const currentPanel = document.getElementById(panelId)
    this.state.selected = id
    if (currentPanel) currentPanel.removeAttribute('hidden')
  }

  click (e) {
    const trigger = Tonic.match(e.target, 'button')
    if (!trigger) return

    this.setVisibility(trigger.id)
  }

  keydown (e) {
    const trigger = Tonic.match(e.target, 'button.tonic--title')
    if (!trigger) return

    const CTRL = e.ctrlKey
    const PAGEUP = e.code === 'PageUp'
    const PAGEDOWN = e.code === 'PageDown'
    const UPARROW = e.code === 'ArrowUp'
    const DOWNARROW = e.code === 'ArrowDown'
    const END = e.metaKey && e.code === 'ArrowDown'
    const HOME = e.metaKey && e.code === 'ArrowUp'

    const ctrlModifier = CTRL && (PAGEUP || PAGEDOWN)
    const triggers = this.qsa('button.tonic--title')

    if ((UPARROW || DOWNARROW) || ctrlModifier) {
      const index = triggers.indexOf(e.target)
      const direction = (PAGEDOWN || DOWNARROW) ? 1 : -1
      const length = triggers.length
      const newIndex = (index + length + direction) % length

      triggers[newIndex].focus()
      e.preventDefault()
    }

    if (HOME || END) {
      switch (e.key) {
        case HOME:
          triggers[0].focus()
          break
        case END:
          triggers[triggers.length - 1].focus()
          break
      }
      e.preventDefault()
    }
  }

  connected () {
    const id = this.state.selected || this.props.selected
    if (!id) return

    setTimeout(() => this.setVisibility(id), 0)
  }

  render () {
    const {
      multiple
    } = this.props

    if (multiple) this.setAttribute('data-allow-multiple', '')

    return this.html`
      ${this.nodes}
    `
  }
}

class TonicAccordionSection extends Tonic {
  static stylesheet () {
    return `
      tonic-accordion-section {
        display: block;
      }

      tonic-accordion-section:not(:last-of-type) {
        border-bottom: 1px solid var(--tonic-border, black);
      }

      tonic-accordion-section .tonic--accordion-header {
        display: flex;
        margin: 0;
      }

      tonic-accordion-section .tonic--accordion-panel[hidden] {
        display: none;
      }

      tonic-accordion-section .tonic--accordion-panel {
        padding: 10px 50px 20px 20px;
      }

      tonic-accordion-section .tonic--accordion-header button {
        font-size: 14px;
        text-align: left;
        padding: 20px;
        position: relative;
        background: transparent;
        color: var(--tonic-primary);
        font-weight: bold;
        border: 0;
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
        outline: none;
        width: 100%;
      }

      tonic-accordion-section .tonic--accordion-header button:focus {
        outline: none;
      }

      tonic-accordion-section .tonic--accordion-header button:focus .tonic--label {
        border-bottom: 2px solid var(--tonic-accent);
      }

      tonic-accordion-section .tonic--accordion-header .tonic--arrow {
        display: block;
        position: absolute;
        top: 0;
        right: 0;
        bottom: 0;
        width: 50px;
      }

      tonic-accordion-section .tonic--accordion-header .tonic--arrow:before {
        content: "";
        width: 8px;
        height: 8px;
        position: absolute;
        top: 50%;
        left: 50%;
        -webkit-transform: translateY(-50%) translateX(-50%) rotate(135deg);
        -moz-transform: translateY(-50%) translateX(-50%) rotate(135deg);
        transform: translateY(-50%) translateX(-50%) rotate(135deg);
        border-top: 1px solid var(--tonic-primary, #333);
        border-right: 1px solid var(--tonic-primary, #333);
      }

      tonic-accordion-section .tonic--accordion-header button[aria-expanded="true"] .tonic--arrow:before {
        -webkit-transform: translateY(-50%) translateX(-50%) rotate(315deg);
        -moz-transform: translateY(-50%) translateX(-50%) rotate(315deg);
        transform: translateY(-50%) translateX(-50%) rotate(315deg);
        margin-top: 3px;
      }
    `
  }

  render () {
    const {
      id,
      name,
      label
    } = this.props

    return this.html`
      <div class="tonic--accordion-header" role="heading">
        <button
          class="tonic--title"
          id="tonic--accordion-header-${id}"
          name="${name}"
          aria-expanded="false"
          aria-controls="tonic--accordion-panel-${id}">
          <span class="tonic--label">${label}</span>
          <span class="tonic--arrow"></span>
        </button>
      </div>
      <div
        class="tonic--accordion-panel"
        id="tonic--accordion-panel-${id}"
        aria-labelledby="tonic--accordion-header-${id}"
        role="region"
        hidden>
        ${this.nodes}
      </div>
    `
  }
}

module.exports = {
  TonicAccordion,
  TonicAccordionSection
}

},{"@optoolco/tonic":12}],2:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicBadge extends Tonic {
  defaults () {
    return {
      count: 0
    }
  }

  get value () {
    return this.state.count
  }

  set value (value) {
    this.state.count = value
    this.reRender()
  }

  static stylesheet () {
    return `
      tonic-badge * {
        box-sizing: border-box;
      }

      tonic-badge .tonic--notifications {
        width: 40px;
        height: 40px;
        text-align: center;
        padding: 10px;
        position: relative;
        background-color: var(--tonic-background, #fff);
        border: 1px solid var(--tonic-input-border, transparent);
        border-radius: 8px;
      }

      tonic-badge span:after {
        content: '';
        width: 8px;
        height: 8px;
        display: none;
        position: absolute;
        top: 7px;
        right: 6px;
        background-color: var(--tonic-notification, #f66);
        border: 2px solid var(--tonic-background, #fff);
        border-radius: 50%;
      }

      tonic-badge .tonic--notifications.tonic--new span:after {
        display: block;
      }

      tonic-badge span {
        color: var(--tonic-primary, #333);
        font: 15px var(--tonic-subheader, 'Arial', sans-serif);
        letter-spacing: 1px;
        text-align: center;
      }
    `
  }

  willConnect () {
    this.state.count = this.props.count
  }

  render () {
    const {
      theme
    } = this.props

    let count = this.state.count

    if (typeof count === 'undefined') {
      count = this.props.count
    }

    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const countText = (count > 99) ? '99' : String(count)
    const classes = ['tonic--notifications']
    if (count > 0) classes.push('tonic--new')

    return this.html`
      <div ... ${{ class: classes.join(' ') }}>
        <span>${countText}</span>
      </div>
    `
  }
}

module.exports = {
  TonicBadge
}

},{"@optoolco/tonic":12}],3:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicButton extends Tonic {
  get value () {
    return this.props.value
  }

  get form () {
    return this.querySelector('button').form
  }

  get disabled () {
    return this.querySelector('button').hasAttribute('disabled')
  }

  set disabled (disabledValue) {
    disabledValue = String(disabledValue)
    this.props.disabled = disabledValue

    const button = this.querySelector('button')
    if (disabledValue === 'true') {
      button.setAttribute('disabled', 'true')
      this.setAttribute('disabled', 'true')
    } else {
      button.removeAttribute('disabled')
      this.removeAttribute('disabled')
    }
  }

  defaults () {
    return {
      height: this.props.type === 'icon' ? '100%' : '34px',
      width: this.props.type === 'icon' ? '100%' : '140px',
      margin: '0px',
      autofocus: 'false',
      async: false,
      radius: '2px',
      borderWidth: '0px',
      textColorDisabled: 'var(--tonic-disabled)',
      backgroundColor: 'transparent'
    }
  }

  static stylesheet () {
    return `
      tonic-button {
        display: inline-block;
        user-select: none;
      }

      tonic-button button {
        color: var(--tonic-button-text, var(--tonic-primary, rgba(54, 57, 61, 1)));
        width: auto;
        font: 12px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: bold;
        font-size: 12px;
        text-transform: uppercase;
        letter-spacing: 1px;
        padding: 6px;
        position: relative;
        background-color: var(--tonic-button-background, transparent);
        transition: background 0.3s ease, color 0.3s ease;
        box-shadow: 0 1px 2px var(--tonic-button-shadow);
        appearance: none;
      }

      tonic-button[type="icon"] button {
        background: none;
        box-shadow: none;
      }

      tonic-button[type="icon"] tonic-icon {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }

      tonic-button button:focus {
        background-color: var(--tonic-button-background-focus, rgba(247, 247, 245, 1));
        outline: none;
      }

      tonic-button button[disabled],
      tonic-button button.tonic--active {
        color: var(--tonic-medium, rgba(153, 157, 160, 1));
        background-color: var(--tonic-button-background-disabled, rgba(247, 247, 245, 1));
      }

      tonic-button button[disabled],
      tonic-button button:active {
        animation-duration: .2s;
        animation-name: tonic--button--activate;
        transition-timing-function: ease;
      }

      @keyframes tonic--button--activate {
        0% {
          transform: scale(1);
        }

        50% {
          transform: scale(0.95);
        }

        100% {
          transform: scale(1);
        }
      }

      tonic-button button:not([disabled]):hover,
      tonic-button button:not(.tonic--loading):hover {
        background-color: var(--tonic-button-background-hover, rgba(54, 57, 61, 1)) !important;
        cursor: pointer;
      }

      tonic-button[disabled="true"],
      tonic-button[disabled="true"] button,
      tonic-button button[disabled] {
        pointer-events: none;
        user-select: none;
        background-color: transparent
      }

      tonic-button button.tonic--loading {
        color: transparent !important;
        transition: all 0.3s ease-in;
        pointer-events: none;
      }

      tonic-button button.tonic--loading:hover {
        color: transparent !important;
        background: var(--tonic-medium, rgba(153, 157, 160, 1)) !important;
      }

      tonic-button button.tonic--loading:before {
        margin-top: -8px;
        margin-left: -8px;
        display: inline-block;
        position: absolute;
        top: 50%;
        left: 50%;
        opacity: 1;
        transform: translateX(-50%) translateY(-50%);
        border: 2px solid var(--tonic-button-text, var(--tonic-primary, black));
        border-radius: 50%;
        border-top-color: transparent;
        animation: spin 1s linear 0s infinite;
        transition: opacity 0.3s ease;
      }

      tonic-button button:before {
        content: '';
        width: 14px;
        height: 14px;
        opacity: 0;
      }

      @keyframes spin {
        from {
          transform: rotate(0deg);
        }
        to {
          transform: rotate(360deg);
        }
      }
    `
  }

  loading (state) {
    const button = this.querySelector('button')
    const method = state ? 'add' : 'remove'
    if (button) button.classList[method]('tonic--loading')
  }

  click (e) {
    const disabled = this.props.disabled === 'true'
    const async = this.props.async === 'true'
    const href = this.props.href

    if (async && !disabled) {
      this.loading(true)
    }

    if (href) {
      const target = this.getAttribute('target')

      if (target && target !== '_self') {
        window.open(href)
      } else {
        window.open(href, '_self')
      }
    }
  }

  styles () {
    const {
      width,
      height,
      margin,
      radius,
      borderWidth
    } = this.props

    return {
      button: {
        width,
        height,
        borderRadius: radius,
        borderWidth: borderWidth
      },
      wrapper: {
        width,
        height,
        margin
      }
    }
  }

  render () {
    const {
      value,
      type,
      disabled,
      autofocus,
      active,
      async,
      fill,
      tabindex,
      size,
      symbolId
    } = this.props

    let classes = []

    if (active) classes.push('tonic--active')
    classes = classes.join(' ')

    if (tabindex) this.removeAttribute('tabindex')

    let content = ''

    if (type === 'icon') {
      content = this.html`
        <tonic-icon
          size="${size || '18px'}"
          fill="${fill || 'var(--tonic-primary)'}"
          symbol-id="${symbolId}"
        ></tonic-icon>
      `
    } else {
      content = this.childNodes
    }

    return this.html`
      <div class="tonic--button--wrapper" styles="wrapper">
        <button ... ${{
          styles: 'button',
          async: String(async),
          disabled: disabled && disabled !== 'false',
          autofocus: autofocus === 'true' ? 'autofocus' : '',
          value,
          type,
          tabindex,
          class: classes
        }}>${content}</button>
      </div>
    `
  }
}

module.exports = { TonicButton }

},{"@optoolco/tonic":12}],4:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicChart extends Tonic {
  static stylesheet () {
    return `
      tonic-chart {
        display: inline-block;
        position: relative;
      }

      tonic-chart canvas {
        display: inline-block;
        position: relative;
      }
    `
  }

  set library (Chart) {
    this.Chart = Chart
  }

  draw (data = {}, options = {}) {
    const root = this.querySelector('canvas')
    const type = this.props.type || options.type

    if (!this.Chart) {
      console.error('No chart constructor found')
      return
    }

    return new this.Chart(root, {
      type,
      options,
      data
    })
  }

  async redraw () {
    return this.connected()
  }

  async fetch (url, opts = {}) {
    if (!url) return {}

    try {
      const res = await window.fetch(url, opts)
      return { data: await res.json() }
    } catch (err) {
      return { err }
    }
  }

  async connected () {
    let data = null
    if (this.props.library) {
      this.Chart = this.props.library
    }

    if (!this.Chart) return

    const options = {
      ...this.props,
      ...this.props.options
    }

    const src = this.props.src

    if (typeof src === 'string') {
      const response = await this.fetch(src)

      if (response.err) {
        console.error(response.err)
        data = {}
      } else {
        data = response.data
      }
    }

    if (src === Object(src)) {
      data = src
    }

    if (data && data.chartData) {
      throw new Error('chartData propery deprecated')
    }

    if (data) {
      this.draw(data, options)
    }
  }

  render () {
    const {
      width,
      height
    } = this.props

    this.style.width = width
    this.style.height = height

    return this.html`
      <canvas ... ${{ width, height }}>
      </canvas>
    `
  }
}

module.exports = { TonicChart }

},{"@optoolco/tonic":12}],5:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicCheckbox extends Tonic {
  constructor () {
    super()
    this._modified = false
  }

  get value () {
    const state = this.state
    const props = this.props

    const propsValue = typeof props.checked !== 'undefined' ? props.checked : props.value
    const stateValue = typeof state.checked !== 'undefined' ? state.checked : state.value
    let value

    if (this._modified) {
      value = typeof stateValue !== 'undefined' ? stateValue : propsValue
    } else {
      value = typeof propsValue !== 'undefined' ? propsValue : stateValue
    }

    return (value === true) || (value === 'true')
  }

  set value (value) {
    this._setValue(value)
  }

  async _setValue (value) {
    this._modified = true
    this.state._changing = true
    const checked = (value === true) || (value === 'true')

    this.state.checked = checked
    this.props.checked = checked
    await this.reRender()
  }

  defaults () {
    return {
      disabled: false,
      size: '18px'
    }
  }

  static stylesheet () {
    return `
      tonic-checkbox .tonic--checkbox--wrapper {
        display: inline-block;
        -webkit-user-select: none;
        -moz-user-select: none;
        user-select: none;
      }

      tonic-checkbox > div {
        height: auto;
        padding: 6px;
      }

      tonic-checkbox input[type="checkbox"] {
        display: none;
      }

      tonic-checkbox input[type="checkbox"][disabled] + label {
        opacity: 0.35;
      }

      tonic-checkbox label {
        color: var(--tonic-primary, #333);
        font: 12px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: 500;
        text-transform: uppercase;
        letter-spacing: 1px;
        display: inline;
        vertical-align: middle;
      }

      tonic-checkbox .tonic--icon {
        position: absolute;
        display: inline-block;
        width: 100%;
        height: 100%;
        background-size: contain;
        margin: 4px;
      }

      tonic-checkbox .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-checkbox label:nth-of-type(2) {
        display: inline-block;
        line-height: 22px;
        margin: 2px 2px 2px 30px;
      }
    `
  }

  change (e) {
    if (
      this.props.virtual === true ||
      this.props.virtual === 'true'
    ) {
      return
    }
    if (this.state._changing) return

    e.stopPropagation()
    this._setValue(!this.value)
  }

  updated () {
    if (this.state._changing) {
      const e = new window.Event('change', { bubbles: true })
      this.dispatchEvent(e)
      delete this.state._changing
    }
  }

  styles () {
    return {
      icon: {
        width: this.props.size,
        height: this.props.size
      }
    }
  }

  renderIcon () {
    const checked = this.value
    const iconState = checked ? 'checked' : 'unchecked'

    return this.html`
      <svg>
        <use ... ${{
          href: `#${iconState}`,
          'xlink:href': `#${iconState}`,
          color: 'var(--tonic-primary, #333)',
          fill: 'var(--tonic-primary, #333)'
        }}>
        </use>
      </svg>
    `
  }

  renderLabel () {
    let {
      id,
      label
    } = this.props

    if (!this.props.label) {
      label = this.nodes
    }

    return this.html`
      <label
        styles="label"
        for="tonic--checkbox--${id}"
      >${label}</label>
    `
  }

  async keydown (e) {
    if (e.code === 'Space') {
      await this._setValue(!this.value)
      this.querySelector('.tonic--checkbox--wrapper').focus()
    }
  }

  render () {
    const {
      id,
      disabled,
      theme,
      title,
      tabindex
    } = this.props

    const checked = this.value
    if (typeof this.state.checked === 'undefined') {
      this.state.checked = checked
    }

    if (tabindex) this.removeAttribute('tabindex')
    if (theme) this.classList.add(`tonic--theme--${theme}`)

    return this.html`
      <div tabindex="0" class="tonic--checkbox--wrapper">
        <input ... ${{
          type: 'checkbox',
          id: `tonic--checkbox--${id}`,
          checked,
          disabled: (disabled === true) || (disabled === 'true'),
          title,
          tabindex
        }}/>
        <label
          for="tonic--checkbox--${id}"
          styles="icon"
          class="tonic--icon"
        >
          ${this.renderIcon()}
        </label>
        ${this.renderLabel()}
      </div>
    `
  }
}

module.exports = { TonicCheckbox }

},{"@optoolco/tonic":12}],6:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicDialog extends Tonic {
  constructor () {
    super()

    this.classList.add('tonic--dialog')
    this.setAttribute('hidden', true)

    if (!document.querySelector('.tonic--dialog--overlay')) {
      const div = document.createElement('div')
      div.classList = 'tonic--dialog--overlay'
      div.textContent = ' '
      document.body.appendChild(div)
    }

    this.closeIcon = document.createElement('div')
    this.closeIcon.className = 'tonic--dialog--close'

    const svgns = 'http://www.w3.org/2000/svg'
    const xlinkns = 'http://www.w3.org/1999/xlink'
    const svg = document.createElementNS(svgns, 'svg')
    const use = document.createElementNS(svgns, 'use')

    this.closeIcon.appendChild(svg)
    svg.appendChild(use)

    use.setAttributeNS(xlinkns, 'href', '#close')
    use.setAttributeNS(xlinkns, 'xlink:href', '#close')

    const iconColor = 'var(--tonic-primary, #333)'
    use.setAttribute('color', iconColor)
    use.setAttribute('fill', iconColor)
  }

  defaults () {
    return {
      width: '450px',
      height: 'auto',
      overlay: true,
      backgroundColor: 'rgba(0, 0, 0, 0.5)'
    }
  }

  _getZIndex () {
    return Array.from(document.querySelectorAll('body *'))
      .map(elt => parseFloat(window.getComputedStyle(elt).zIndex))
      .reduce((z, highest = Number.MIN_SAFE_INTEGER) =>
        isNaN(z) || z < highest ? highest : z
      )
  }

  static stylesheet () {
    return `
      .tonic--dialog {
        box-shadow: 0px 6px 35px 3px rgba(0, 0, 0, 0.2);
        background: var(--tonic-window);
        border: 1px solid var(--tonic-border);
        border-radius: 6px;
        position: fixed;
        overflow: hidden;
        top: 50%;
        left: 50%;
        z-index: -1;
        opacity: 0;
        transition: z-index .25s;
        transform: translate(-50%, -50%) scale(0.88);
        will-change: transform;
      }

      .tonic--dialog.tonic--show {
        transform: translate(-50%, -50%) scale(1);
        opacity: 1;
        animation-duration: .25s;
        animation-name: tonic--dialog--show;
        transition-timing-function: ease;
      }

      .tonic--dialog.tonic--hide {
        transform: translate(-50%, -50%) scale(0.88);
        opacity: 0;
        animation-duration: .2s;
        animation-name: tonic--dialog--hide;
        transition-timing-function: ease;
      }

      .tonic--dialog > .tonic--dialog--close {
        width: 25px;
        height: 25px;
        position: absolute;
        top: 10px;
        right: 10px;
        cursor: pointer;
      }

      .tonic--dialog > .tonic--dialog--close svg {
        width: inherit;
        height: inherit;
      }

      @keyframes tonic--dialog--show {
        from {
          transform: translate(-50%, -50%) scale(0.88);
          opacity: 0;
        }

        to {
          opacity: 1;
          transform: translate(-50%, -50%) scale(1);
        }
      }

      @keyframes tonic--dialog--hide {
        from {
          transform: translate(-50%, -50%) scale(1);
          opacity: 1;
        }

        to {
          opacity: 0;
          transform: translate(-50%, -50%) scale(0.88);
        }
      }

      .tonic--dialog--overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        opacity: 0;
        z-index: -1;
        transition: all 0.2s;
        background: var(--tonic-overlay);
      }

      .tonic--dialog--overlay.tonic--show {
        opacity: 1;
      }
    `
  }

  show () {
    const z = this._getZIndex()
    this.appendChild(this.closeIcon)
    this.removeAttribute('hidden')

    const overlay = document.querySelector('.tonic--dialog--overlay')
    overlay.classList.add('tonic--show')
    this.style.zIndex = z + 100
    overlay.style.zIndex = z

    return new Promise(resolve => {
      if (this.props.widthMobile && document.body.clientWidth < 500) {
        this.props.width = this.props.widthMobile
      }

      this.style.width = this.props.width
      this.style.height = this.props.height

      const done = () => {
        clearTimeout(timer)
        resolve()
      }

      const timer = setTimeout(done, 512)
      this.addEventListener('animationend', done, { once: true })

      this.classList.remove('tonic--hide')
      this.classList.add('tonic--show')

      this._escapeHandler = e => {
        if (e.keyCode === 27) this.hide()
      }

      document.addEventListener('keyup', this._escapeHandler)
    })
  }

  hide () {
    const overlay = document.querySelector('.tonic--dialog--overlay')
    overlay.classList.remove('tonic--show')
    overlay.style.zIndex = -1

    return new Promise(resolve => {
      this.style.zIndex = -1
      document.removeEventListener('keyup', this._escapeHandler)

      const done = () => {
        clearTimeout(timer)
        this.setAttribute('hidden', true)
        resolve()
      }

      const timer = setTimeout(done, 512)
      this.addEventListener('animationend', done, { once: true })

      this.classList.remove('tonic--show')
      this.classList.add('tonic--hide')
    })
  }

  event (eventName) {
    const that = this

    return {
      then (resolve) {
        const resolver = e => {
          if (e.keyCode === 27) resolve({})
        }

        const listener = event => {
          const close = Tonic.match(event.target, '.tonic--dialog--close')
          const value = Tonic.match(event.target, '[value]')

          if (close || value) {
            that.removeEventListener(eventName, listener)
            document.removeEventListener('keyup', resolver)
          }

          if (close) return resolve({})
          if (value) resolve({ [event.target.value]: true })
        }

        document.addEventListener('keyup', resolver)

        that.addEventListener(eventName, listener)
      }
    }
  }

  click (e) {
    if (Tonic.match(e.target, '.tonic--dialog--close')) {
      this.hide()
    }
  }

  updated () {
    this.appendChild(this.closeIcon)
  }

  render () {
    return this.html`
      ${this.children}
    `
  }
}

module.exports = { TonicDialog }

},{"@optoolco/tonic":12}],7:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicForm extends Tonic {
  static isNumber (s) {
    return !isNaN(Number(s))
  }

  static getPropertyValue (o, path) {
    if (!path) return null

    const parts = path.split('.')
    let value = o

    for (const p of parts) {
      if (!value) return null
      value = value[p]
    }

    return value
  }

  static setPropertyValue (o, path, v) {
    if (!path) return

    const parts = path.split('.')
    let value = o

    const last = parts.pop()
    if (!last) return

    for (let i = 0; i < parts.length; i++) {
      const p = parts[i]
      const next = parts[i + 1] || last

      if (!value[p]) {
        value[p] = TonicForm.isNumber(next) ? [] : {}
      }

      value = value[p]
    }

    value[last] = v
    return o
  }

  setData (data) {
    this.state = data
  }

  getData () {
    return this.value
  }

  getElements () {
    return [...this.querySelectorAll('[data-key]')]
  }

  get value () {
    const elements = this.getElements()

    for (const element of elements) {
      TonicForm.setPropertyValue(this.state, element.dataset.key, element.value)
    }

    return this.state
  }

  set value (data) {
    if (typeof data !== 'object') return

    const elements = this.getElements()

    for (const element of elements) {
      const value = TonicForm.getPropertyValue(data, element.dataset.key)
      if (!value) continue

      element.value = value
    }

    this.state = data
  }

  setValid () {
    const elements = this.getElements()

    for (const element of elements) {
      if (element.setValid) {
        element.state.edited = false
        element.removeAttribute('edited')
        element.setValid()
      }
    }
  }

  input (e) {
    this.change(e)
  }

  change (e) {
    const el = Tonic.match(e.target, '[data-key]')
    if (!el) return

    TonicForm.setPropertyValue(this.state, el.dataset.key, el.value)
  }

  validate ({ decorate = true } = {}) {
    this.getData()
    const elements = this.getElements()
    let isValid = true

    for (const el of elements) {
      if (!el.setInvalid) continue

      let selector = ''

      if (el.tagName === 'TONIC-INPUT') {
        selector = 'input'
      } else if (el.tagName === 'TONIC-TEXTAREA') {
        selector = 'textarea'
      } else if (el.tagName === 'TONIC-SELECT') {
        selector = 'select'
      } else if (el.tagName === 'TONIC-CHECKBOX') {
        selector = 'checkbox'
      } else if (el.tagName === 'TONIC-TOGGLE') {
        selector = 'checkbox'
      }

      const input = selector ? el.querySelector(selector) : el
      if (!input.checkValidity) continue

      input.checkValidity()

      for (const key in input.validity) {
        if ((key === 'valid') || (key === 'customError') || !input.validity[key]) {
          if (decorate) el.setValid()
          continue
        }

        if (decorate) el.setInvalid(el.props.errorMessage || 'Required')
        isValid = false
      }
    }

    return isValid
  }

  connected () {
    if (!this.props.fill) return

    const elements = this.getElements()

    for (const element of elements) {
      const key = element.dataset.key
      const value = TonicForm.getPropertyValue(this.state, key)
      element.value = value || element.value || ''
    }
  }

  render () {
    return this.html`
      ${this.childNodes}
    `
  }
}

module.exports = { TonicForm }

},{"@optoolco/tonic":12}],8:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicIcon extends Tonic {
  defaults () {
    return {
      size: '25px',
      fill: 'var(--tonic-primary, #333)'
    }
  }

  static stylesheet () {
    return `
      tonic-icon {
        vertical-align: middle;
      }

      tonic-icon svg path {
        fill: inherit;
      }
    `
  }

  styles () {
    const {
      size
    } = this.props

    return {
      icon: {
        width: size,
        height: size
      }
    }
  }

  render () {
    const {
      symbolId,
      size,
      fill,
      theme,
      src,
      tabindex
    } = this.props

    if (tabindex) this.removeAttribute('tabindex')
    if (theme) this.classList.add(`tonic--theme--${theme}`)

    return this.html`
      <svg ... ${{
        styles: 'icon',
        tabindex
      }}>
        <use ... ${{
          href: `${src || ''}#${symbolId}`,
          'xlink:href': `${src || ''}#${symbolId}`,
          width: size,
          fill,
          color: fill,
          height: size
        }}>
      </svg>
    `
  }
}

module.exports = { TonicIcon }

},{"@optoolco/tonic":12}],9:[function(require,module,exports){
let Tonic
try {
  Tonic = require('@optoolco/tonic')
} catch (err) {
  console.error('Missing dependency. Try `npm install @optoolco/tonic`.')
  throw err
}

const version = Tonic.version
const major = version ? version.split('.')[0] : '0'

if (parseInt(major, 10) < 12) {
  console.error('Out of date dependency. Try `npm install @optoolco/tonic@12`.')
  throw new Error('Invalid Tonic version. requires at least v12')
}

const { TonicAccordion, TonicAccordionSection } = require('./accordion')
const { TonicBadge } = require('./badge')
const { TonicButton } = require('./button')
const { TonicChart } = require('./chart')
const { TonicCheckbox } = require('./checkbox')
const { TonicDialog } = require('./dialog')
const { TonicForm } = require('./form')
const { TonicIcon } = require('./icon')
const { TonicInput } = require('./input')
const { TonicLoader } = require('./loader')
const { TonicPanel } = require('./panel')
const { TonicPopover } = require('./popover')
const { TonicProfileImage } = require('./profile-image')
const { TonicProgressBar } = require('./progress-bar')
const { TonicRange } = require('./range')
const { TonicRelativeTime } = require('./relative-time')
const { TonicRouter } = require('./router')
const { TonicSelect } = require('./select')
const { TonicSprite } = require('./sprite')
const { TonicSplit, TonicSplitLeft, TonicSplitRight, TonicSplitTop, TonicSplitBottom } = require('./split')
const { TonicTabs, TonicTab, TonicTabPanel } = require('./tabs')
const { TonicTextarea } = require('./textarea')
const { TonicTooltip } = require('./tooltip')
const { TonicToasterInline } = require('./toaster-inline')
const { TonicToaster } = require('./toaster')
const { TonicToggle } = require('./toggle')

let once = false

//
// An example collection of components.
//
module.exports = components
// For supporting unpkg / dist / jsfiddle.
components.Tonic = Tonic

function components (Tonic) {
  if (once) {
    return
  }
  once = true

  Tonic.add(TonicAccordion)
  Tonic.add(TonicAccordionSection)
  Tonic.add(TonicBadge)
  Tonic.add(TonicButton)
  Tonic.add(TonicChart)
  Tonic.add(TonicCheckbox)
  Tonic.add(TonicDialog)
  Tonic.add(TonicForm)
  Tonic.add(TonicInput)
  Tonic.add(TonicIcon)
  Tonic.add(TonicLoader)
  Tonic.add(TonicPanel)
  Tonic.add(TonicPopover)
  Tonic.add(TonicProfileImage)
  Tonic.add(TonicProgressBar)
  Tonic.add(TonicRange)
  Tonic.add(TonicRelativeTime)
  Tonic.add(TonicRouter)
  Tonic.add(TonicSelect)
  Tonic.add(TonicSprite)
  Tonic.add(TonicSplit)
  Tonic.add(TonicSplitLeft)
  Tonic.add(TonicSplitRight)
  Tonic.add(TonicSplitTop)
  Tonic.add(TonicSplitBottom)
  Tonic.add(TonicTabPanel)
  Tonic.add(TonicTab)
  Tonic.add(TonicTabs)
  Tonic.add(TonicTextarea)
  Tonic.add(TonicTooltip)
  Tonic.add(TonicToasterInline)
  Tonic.add(TonicToaster)
  Tonic.add(TonicToggle)
}

},{"./accordion":1,"./badge":2,"./button":3,"./chart":4,"./checkbox":5,"./dialog":6,"./form":7,"./icon":8,"./input":10,"./loader":11,"./panel":14,"./popover":15,"./profile-image":16,"./progress-bar":17,"./range":18,"./relative-time":19,"./router":20,"./select":21,"./split":22,"./sprite":23,"./tabs":24,"./textarea":25,"./toaster":27,"./toaster-inline":26,"./toggle":28,"./tooltip":29,"@optoolco/tonic":12}],10:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicInput extends Tonic {
  constructor () {
    super()
    this._modified = false
  }

  defaults () {
    return {
      type: 'text',
      placeholder: '',
      color: 'var(--tonic-primary)',
      spellcheck: false,
      ariaInvalid: false,
      invalid: false,
      radius: '3px',
      disabled: false,
      position: 'left'
    }
  }

  get form () {
    return this.querySelector('input').form
  }

  get value () {
    if (this._modified) {
      return typeof this.state.value === 'string'
        ? this.state.value : this.props.value
    } else {
      return typeof this.props.value === 'string'
        ? this.props.value : this.state.value
    }
  }

  set value (value) {
    this._modified = true
    this.querySelector('input').value = value
    this.state.value = value
  }

  setValid () {
    const input = this.querySelector('input')
    if (!input) return

    input.setCustomValidity('')
    input.removeAttribute('invalid')
  }

  setInvalid (msg) {
    const input = this.querySelector('input')
    if (!input) return

    this.setAttribute('edited', true)
    this.state.edited = true

    msg = msg || this.props.errorMessage

    input.setCustomValidity(msg)
    input.setAttribute('invalid', msg)
    const span = this.querySelector('.tonic--invalid span')
    if (!span) return

    span.textContent = msg
  }

  static stylesheet () {
    return `
      tonic-input .tonic--wrapper {
        position: relative;
      }

      tonic-input[symbol-id] .tonic--right tonic-icon,
      tonic-input[src] .tonic--right tonic-icon {
        right: 6px;
      }

      tonic-input[symbol-id] .tonic--right input,
      tonic-input[src] .tonic--right input {
        padding-right: 40px;
      }

      tonic-input[symbol-id] .tonic--left tonic-icon,
      tonic-input[src] .tonic--left tonic-icon {
        left: 6px;
      }

      tonic-input[symbol-id] .tonic--left input,
      tonic-input[src] .tonic--left input {
        padding-left: 40px;
      }

      tonic-input[symbol-id] tonic-icon,
      tonic-input[src] tonic-icon {
        position: absolute;
        bottom: 6px;
      }

      tonic-input label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
      }

      tonic-input input {
        color: var(--tonic-primary, #333);
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        padding: 8px;
        background-color: var(--tonic-input-background, var(--tonic-background, transparent));
        border: 1px solid var(--tonic-border, #ccc);
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
      }

      tonic-input input:focus {
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
        outline: none;
      }

      tonic-input[edited] input[invalid]:focus,
      tonic-input[edited] input:invalid:focus,
      tonic-input[edited] input[invalid],
      tonic-input[edited] input:invalid {
        border-color: var(--tonic-error, #f66);
      }

      tonic-input[edited] input[invalid] ~ .tonic--invalid,
      tonic-input[edited] input:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-input input[disabled] {
        background-color: transparent;
      }

      tonic-input[label] .tonic--invalid {
        margin-bottom: -13px;
      }

      tonic-input .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
      }

      tonic-input .tonic--invalid span {
        color: white;
        padding: 2px 6px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-input .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }
    `
  }

  renderLabel () {
    if (!this.props.label) return ''
    return this.html`
      <label
        for="tonic--input_${this.props.id}"
      >${this.props.label}</label>
    `
  }

  renderIcon () {
    if (!this.props.src && !this.props.symbolId) return ''

    const opts = {}

    if (this.props.src) {
      opts.src = this.props.src
    } else if (this.props.symbolId) {
      opts.symbolId = this.props.symbolId
    }

    if (this.props.color) {
      opts.color = this.props.color
    }

    if (this.props.fill) {
      opts.fill = this.props.fill
    }

    return this.html`
      <tonic-icon ...${opts}>
      </tonic-icon>
    `
  }

  setupEvents () {
    const input = this.querySelector('input')

    const relay = name => {
      this.dispatchEvent(new window.CustomEvent(name, { bubbles: true }))
    }

    input.addEventListener('focus', e => {
      this.state.focus = true
      relay('focus')
    })

    input.addEventListener('blur', e => {
      if (this.state.rerendering) return
      this.state.focus = false
      relay('blur')
    })

    input.addEventListener('input', e => {
      this._modified = true
      this.state.edited = true
      this.setAttribute('edited', true)
      this.state.value = e.target.value
      this.state.pos = e.target.selectionStart
    })

    const state = this.state
    if (!state.focus) return

    input.focus()

    try {
      input.setSelectionRange(state.pos, state.pos)
    } catch (err) {
      console.warn(err)
    }
  }

  updated () {
    this.setupEvents()

    setTimeout(() => {
      if (this.props.invalid) {
        this.setInvalid(this.props.errorMessage)
      } else {
        this.setValid()
      }
    }, 32)

    this.state.rerendering = false
  }

  async reRender (...args) {
    this.state.rerendering = true
    return super.reRender(...args)
  }

  connected () {
    this.updated()
  }

  styles () {
    const {
      width,
      height,
      radius,
      padding
    } = this.props

    return {
      wrapper: {
        width
      },
      input: {
        width: '100%',
        height,
        borderRadius: radius,
        padding
      }
    }
  }

  render () {
    const {
      ariaInvalid,
      ariaLabelledby,
      disabled,
      height,
      label,
      max,
      maxlength,
      min,
      minlength,
      name,
      pattern,
      placeholder,
      position,
      readonly,
      required,
      spellcheck,
      tabindex,
      theme,
      title,
      type
    } = this.props

    if (ariaLabelledby) this.removeAttribute('ariaLabelledby')
    if (height) this.style.height = height
    if (name) this.removeAttribute('name')
    if (tabindex) this.removeAttribute('tabindex')
    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const value = this.value

    const errorMessage = this.props.errorMessage ||
      this.props.errorMessage || 'Invalid'

    const classes = ['tonic--wrapper']
    if (position) classes.push(`tonic--${position}`)

    const list = this.elements.length
      ? this.props.id + '_datalist'
      : null

    const attributes = {
      ariaInvalid,
      ariaLabel: label,
      'aria-labelledby': ariaLabelledby,
      disabled: disabled === 'true',
      max,
      maxlength,
      min,
      minlength,
      name,
      pattern,
      placeholder,
      position,
      readonly: readonly === 'true',
      required: required === 'true',
      spellcheck,
      tabindex,
      title,
      value,
      list
    }

    if (this.state.edited) {
      this.setAttribute('edited', true)
    }

    let datalist = ''

    if (list) {
      datalist = this.html`
        <datalist id="${list}">
          ${this.elements}
        </datalist>
      `
    }

    return this.html`
      <div class="${classes.join(' ')}" styles="wrapper">
        ${this.renderLabel()}
        ${this.renderIcon()}

        <input ... ${{
          styles: 'input',
          type,
          id: `tonic--input_${this.props.id}`,
          ...attributes
        }}/>

        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${errorMessage}</span>
        </div>

        ${datalist}
      </div>
    `
  }
}

module.exports = { TonicInput }

},{"@optoolco/tonic":12}],11:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicLoader extends Tonic {
  constructor () {
    super()
    this.attachShadow({ mode: 'open' })
  }

  stylesheet () {
    return `
      .outer {
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
      }

      .inner {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
      }
    `
  }

  render () {
    return this.html`
      <div class="outer">
        <div class="inner">
          <svg version="1.1" id="loader-1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
           width="40px" height="40px" viewBox="0 0 40 40" enable-background="new 0 0 40 40" xml:space="preserve">
          <path opacity="0.2" fill="#000" d="M20.201,5.169c-8.254,0-14.946,6.692-14.946,14.946c0,8.255,6.692,14.946,14.946,14.946
            s14.946-6.691,14.946-14.946C35.146,11.861,28.455,5.169,20.201,5.169z M20.201,31.749c-6.425,0-11.634-5.208-11.634-11.634
            c0-6.425,5.209-11.634,11.634-11.634c6.425,0,11.633,5.209,11.633,11.634C31.834,26.541,26.626,31.749,20.201,31.749z"/>
          <path fill="#000" d="M26.013,10.047l1.654-2.866c-2.198-1.272-4.743-2.012-7.466-2.012h0v3.312h0
            C22.32,8.481,24.301,9.057,26.013,10.047z">
            <animateTransform attributeType="xml"
              attributeName="transform"
              type="rotate"
              from="0 20 20"
              to="360 20 20"
              dur="0.8s"
              repeatCount="indefinite"/>
            </path>
          </svg>
        </div>
      </div>
    `
  }
}

module.exports = { TonicLoader }

},{"@optoolco/tonic":12}],12:[function(require,module,exports){
class TonicTemplate {
  constructor (rawText, templateStrings, unsafe) {
    this.isTonicTemplate = true
    this.unsafe = unsafe
    this.rawText = rawText
    this.templateStrings = templateStrings
  }

  valueOf () { return this.rawText }
  toString () { return this.rawText }
}

class Tonic extends window.HTMLElement {
  constructor () {
    super()
    const state = Tonic._states[super.id]
    delete Tonic._states[super.id]
    this._state = state || {}
    this.preventRenderOnReconnect = false
    this.props = {}
    this.elements = [...this.children]
    this.elements.__children__ = true
    this.nodes = [...this.childNodes]
    this.nodes.__children__ = true
    this._events()
  }

  static _createId () {
    return `tonic${Tonic._index++}`
  }

  static _splitName (s) {
    return s.match(/[A-Z][a-z]*/g).join('-')
  }

  static _normalizeAttrs (o, x = {}) {
    [...o].forEach(o => (x[o.name] = o.value))
    return x
  }

  _checkId () {
    const _id = super.id
    if (!_id) {
      const html = this.outerHTML.replace(this.innerHTML, '...')
      throw new Error(`Component: ${html} has no id`)
    }
    return _id
  }

  get state () {
    return (this._checkId(), this._state)
  }

  set state (newState) {
    this._state = (this._checkId(), newState)
  }

  get id () { return this._checkId() }

  set id (newId) { super.id = newId }

  _events () {
    const hp = Object.getOwnPropertyNames(window.HTMLElement.prototype)
    for (const p of this._props) {
      if (hp.indexOf('on' + p) === -1) continue
      this.addEventListener(p, this)
    }
  }

  _prop (o) {
    const id = this._id
    const p = `__${id}__${Tonic._createId()}__`
    Tonic._data[id] = Tonic._data[id] || {}
    Tonic._data[id][p] = o
    return p
  }

  _placehold (r) {
    const id = this._id
    const ref = `placehold:${id}:${Tonic._createId()}__`
    Tonic._children[id] = Tonic._children[id] || {}
    Tonic._children[id][ref] = r
    return ref
  }

  static match (el, s) {
    if (!el.matches) el = el.parentElement
    return el.matches(s) ? el : el.closest(s)
  }

  static getPropertyNames (proto) {
    const props = []
    while (proto && proto !== Tonic.prototype) {
      props.push(...Object.getOwnPropertyNames(proto))
      proto = Object.getPrototypeOf(proto)
    }
    return props
  }

  static add (c, htmlName) {
    const hasValidName = htmlName || (c.name && c.name.length > 1)
    if (!hasValidName) {
      throw Error('Mangling. https://bit.ly/2TkJ6zP')
    }

    if (!htmlName) htmlName = Tonic._splitName(c.name).toLowerCase()
    if (window.customElements.get(htmlName)) {
      throw new Error(`Cannot Tonic.add(${c.name}, '${htmlName}') twice`)
    }

    if (!c.prototype.isTonicComponent) {
      const tmp = { [c.name]: class extends Tonic {} }[c.name]
      tmp.prototype.render = c
      c = tmp
    }

    c.prototype._props = Tonic.getPropertyNames(c.prototype)

    Tonic._reg[htmlName] = c
    Tonic._tags = Object.keys(Tonic._reg).join()
    window.customElements.define(htmlName, c)

    if (c.stylesheet) {
      Tonic.registerStyles(c.stylesheet)
    }
  }

  static registerStyles (stylesheetFn) {
    if (Tonic._stylesheetRegistry.includes(stylesheetFn)) return
    Tonic._stylesheetRegistry.push(stylesheetFn)

    const styleNode = document.createElement('style')
    if (Tonic.nonce) styleNode.setAttribute('nonce', Tonic.nonce)
    styleNode.appendChild(document.createTextNode(stylesheetFn()))
    if (document.head) document.head.appendChild(styleNode)
  }

  static escape (s) {
    return s.replace(Tonic.ESC, c => Tonic.MAP[c])
  }

  static unsafeRawString (s, templateStrings) {
    return new TonicTemplate(s, templateStrings, true)
  }

  dispatch (eventName, detail = null) {
    const opts = { bubbles: true, detail }
    this.dispatchEvent(new window.CustomEvent(eventName, opts))
  }

  html (strings, ...values) {
    const refs = o => {
      if (o && o.__children__) return this._placehold(o)
      if (o && o.isTonicTemplate) return o.rawText
      switch (Object.prototype.toString.call(o)) {
        case '[object HTMLCollection]':
        case '[object NodeList]': return this._placehold([...o])
        case '[object Array]':
          if (o.every(x => x.isTonicTemplate && !x.unsafe)) {
            return new TonicTemplate(o.join('\n'), null, false)
          }
          return this._prop(o)
        case '[object Object]':
        case '[object Function]': return this._prop(o)
        case '[object NamedNodeMap]':
          return this._prop(Tonic._normalizeAttrs(o))
        case '[object Number]': return `${o}__float`
        case '[object String]': return Tonic.escape(o)
        case '[object Boolean]': return `${o}__boolean`
        case '[object Null]': return `${o}__null`
        case '[object HTMLElement]':
          return this._placehold([o])
      }
      if (
        typeof o === 'object' && o && o.nodeType === 1 &&
        typeof o.cloneNode === 'function'
      ) {
        return this._placehold([o])
      }
      return o
    }

    const out = []
    for (let i = 0; i < strings.length - 1; i++) {
      out.push(strings[i], refs(values[i]))
    }
    out.push(strings[strings.length - 1])

    const htmlStr = out.join('').replace(Tonic.SPREAD, (_, p) => {
      const o = Tonic._data[p.split('__')[1]][p]
      return Object.entries(o).map(([key, value]) => {
        const k = key.replace(/([a-z])([A-Z])/g, '$1-$2').toLowerCase()
        if (value === true) return k
        else if (value) return `${k}="${Tonic.escape(String(value))}"`
        else return ''
      }).filter(Boolean).join(' ')
    })
    return new TonicTemplate(htmlStr, strings, false)
  }

  scheduleReRender (oldProps) {
    if (this.pendingReRender) return this.pendingReRender

    this.pendingReRender = new Promise(resolve => window.setTimeout(() => {
      if (!this.isInDocument(this.shadowRoot || this)) return
      const p = this._set(this.shadowRoot || this, this.render)
      this.pendingReRender = null

      if (p && p.then) {
        return p.then(() => {
          this.updated && this.updated(oldProps)
          resolve()
        })
      }

      this.updated && this.updated(oldProps)
      resolve()
    }, 0))

    return this.pendingReRender
  }

  reRender (o = this.props) {
    const oldProps = { ...this.props }
    this.props = typeof o === 'function' ? o(oldProps) : o
    return this.scheduleReRender(oldProps)
  }

  handleEvent (e) {
    this[e.type](e)
  }

  _drainIterator (target, iterator) {
    return iterator.next().then((result) => {
      this._set(target, null, result.value)
      if (result.done) return
      return this._drainIterator(target, iterator)
    })
  }

  _set (target, render, content = '') {
    for (const node of target.querySelectorAll(Tonic._tags)) {
      if (!node.isTonicComponent) continue

      const id = node.getAttribute('id')
      if (!id || !Tonic._refIds.includes(id)) continue
      Tonic._states[id] = node.state
    }

    if (render instanceof Tonic.AsyncFunction) {
      return render.call(this).then(content => this._apply(target, content))
    } else if (render instanceof Tonic.AsyncFunctionGenerator) {
      return this._drainIterator(target, render.call(this))
    } else if (render === null) {
      this._apply(target, content)
    } else if (render instanceof Function) {
      this._apply(target, render.call(this) || '')
    }
  }

  _apply (target, content) {
    if (content && content.isTonicTemplate) {
      content = content.rawText
    } else if (typeof content === 'string') {
      content = Tonic.escape(content)
    }

    if (typeof content === 'string') {
      if (this.stylesheet) {
        content = `<style nonce=${Tonic.nonce || ''}>${this.stylesheet()}</style>${content}`
      }

      target.innerHTML = content

      if (this.styles) {
        const styles = this.styles()
        for (const node of target.querySelectorAll('[styles]')) {
          for (const s of node.getAttribute('styles').split(/\s+/)) {
            Object.assign(node.style, styles[s.trim()])
          }
        }
      }

      const children = Tonic._children[this._id] || {}

      const walk = (node, fn) => {
        if (node.nodeType === 3) {
          const id = node.textContent.trim()
          if (children[id]) fn(node, children[id], id)
        }

        const childNodes = node.childNodes
        if (!childNodes) return

        for (let i = 0; i < childNodes.length; i++) {
          walk(childNodes[i], fn)
        }
      }

      walk(target, (node, children, id) => {
        for (const child of children) {
          node.parentNode.insertBefore(child, node)
        }
        delete Tonic._children[this._id][id]
        node.parentNode.removeChild(node)
      })
    } else {
      target.innerHTML = ''
      target.appendChild(content.cloneNode(true))
    }
  }

  connectedCallback () {
    this.root = this.shadowRoot || this // here for back compat

    if (super.id && !Tonic._refIds.includes(super.id)) {
      Tonic._refIds.push(super.id)
    }
    const cc = s => s.replace(/-(.)/g, (_, m) => m.toUpperCase())

    for (const { name: _name, value } of this.attributes) {
      const name = cc(_name)
      const p = this.props[name] = value

      if (/__\w+__\w+__/.test(p)) {
        const { 1: root } = p.split('__')
        this.props[name] = Tonic._data[root][p]
      } else if (/\d+__float/.test(p)) {
        this.props[name] = parseFloat(p, 10)
      } else if (p === 'null__null') {
        this.props[name] = null
      } else if (/\w+__boolean/.test(p)) {
        this.props[name] = p.includes('true')
      } else if (/placehold:\w+:\w+__/.test(p)) {
        const { 1: root } = p.split(':')
        this.props[name] = Tonic._children[root][p][0]
      }
    }

    this.props = Object.assign(
      this.defaults ? this.defaults() : {},
      this.props
    )

    this._id = this._id || Tonic._createId()

    this.willConnect && this.willConnect()

    if (!this.isInDocument(this.root)) return
    if (!this.preventRenderOnReconnect) {
      if (!this._source) {
        this._source = this.innerHTML
      } else {
        this.innerHTML = this._source
      }
      const p = this._set(this.root, this.render)
      if (p && p.then) return p.then(() => this.connected && this.connected())
    }

    this.connected && this.connected()
  }

  isInDocument (target) {
    const root = target.getRootNode()
    return root === document || root.toString() === '[object ShadowRoot]'
  }

  disconnectedCallback () {
    this.disconnected && this.disconnected()
    delete Tonic._data[this._id]
    delete Tonic._children[this._id]
  }
}

Tonic.prototype.isTonicComponent = true

Object.assign(Tonic, {
  _tags: '',
  _refIds: [],
  _data: {},
  _states: {},
  _children: {},
  _reg: {},
  _stylesheetRegistry: [],
  _index: 0,
  version: typeof require !== 'undefined' ? require('./package').version : null,
  SPREAD: /\.\.\.\s?(__\w+__\w+__)/g,
  ESC: /["&'<>`/]/g,
  AsyncFunctionGenerator: async function * () {}.constructor,
  AsyncFunction: async function () {}.constructor,
  MAP: { '"': '&quot;', '&': '&amp;', '\'': '&#x27;', '<': '&lt;', '>': '&gt;', '`': '&#x60;', '/': '&#x2F;' }
})

if (typeof module === 'object') module.exports = Tonic

},{"./package":13}],13:[function(require,module,exports){
module.exports={
  "_from": "@optoolco/tonic@13.1.3",
  "_id": "@optoolco/tonic@13.1.3",
  "_inBundle": false,
  "_integrity": "sha512-GECzc4Od1fXwrHHqpys7BqUib2kZOjNyoDEhikY5TjJcgBpP5SkgYjf4I8E7Y4TLlQ6jGechB+g7Sknwz5yZOA==",
  "_location": "/@optoolco/tonic",
  "_phantomChildren": {},
  "_requested": {
    "type": "version",
    "registry": true,
    "raw": "@optoolco/tonic@13.1.3",
    "name": "@optoolco/tonic",
    "escapedName": "@optoolco%2ftonic",
    "scope": "@optoolco",
    "rawSpec": "13.1.3",
    "saveSpec": null,
    "fetchSpec": "13.1.3"
  },
  "_requiredBy": [
    "#DEV:/",
    "#USER"
  ],
  "_resolved": "https://registry.npmjs.org/@optoolco/tonic/-/tonic-13.1.3.tgz",
  "_shasum": "62e53a736df2fc8b9aeaade8c57b20bfd5206ce9",
  "_spec": "@optoolco/tonic@13.1.3",
  "_where": "/home/raynos/optoolco/components",
  "author": {
    "name": "optoolco"
  },
  "bugs": {
    "url": "https://github.com/optoolco/tonic/issues"
  },
  "bundleDependencies": false,
  "dependencies": {},
  "deprecated": false,
  "description": "A composable component inspired by React.",
  "devDependencies": {
    "benchmark": "^2.1.4",
    "browserify": "^16.2.2",
    "raynos-tape-puppet": "0.1.7-raynos2",
    "standard": "14.3.1",
    "tape": "^4.11.0",
    "terser": "^4.0.2",
    "uuid": "3.3.3"
  },
  "directories": {
    "test": "test"
  },
  "homepage": "https://github.com/optoolco/tonic#readme",
  "license": "MIT",
  "name": "@optoolco/tonic",
  "repository": {
    "type": "git",
    "url": "git+https://github.com/optoolco/tonic.git"
  },
  "scripts": {
    "build:demo": "browserify --bare ./demo > ./docs/bundle.js",
    "minify": "terser index.js -c unused,dead_code,hoist_vars,loops=false,hoist_props=true,hoist_funs,toplevel,keep_classnames,keep_fargs=false -o dist/tonic.min.js",
    "test": "npm run minify && browserify test/index.js | tape-puppet"
  },
  "version": "13.1.3"
}

},{}],14:[function(require,module,exports){
const { TonicDialog } = require('../dialog')

class TonicPanel extends TonicDialog {
  constructor () {
    super()

    this.classList.add('tonic--panel')
  }

  defaults () {
    return {
      position: 'right'
    }
  }

  stylesheet () {
    const {
      width,
      position
    } = this.props

    const range = [0, width]

    if (position !== 'right') {
      range[1] = `-${width}`
    }

    return `
      .tonic--dialog.tonic--panel,
      .tonic--dialog.tonic--show.tonic--panel {
        left: unset;
        border-radius: 0;
        top: 0;
        width: ${width};
        bottom: 0;
        ${position}: 0;
        transform: translateX(${range[0]});
        animation-name: tonic--panel--show;
      }

      .tonic--dialog.tonic--hide.tonic--panel {
        opacity: 0;
        transform: translateX(${width});
        animation-name: tonic--panel--hide;
      }

      @keyframes tonic--panel--show {
        from {
          opacity: 0;
          transform: translateX(${range[1]});
        }

        to {
          opacity: 1;
          transform: translateX(${range[0]});
        }
      }

      @keyframes tonic--panel--hide {
        from {
          opacity: 1;
          transform: translateX(${range[0]});
        }

        to {
          opacity: 0;
          transform: translateX(${range[1]});
        }
      }
    `
  }
}

module.exports = { TonicPanel }

},{"../dialog":6}],15:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicPopover extends Tonic {
  constructor () {
    super()

    const target = this.getAttribute('for')
    const el = document.getElementById(target)

    el.addEventListener('click', e => this.show(el))
  }

  defaults (props) {
    return {
      width: 'auto',
      height: 'auto',
      padding: '15px',
      margin: 10,
      position: 'bottomleft'
    }
  }

  static stylesheet () {
    return `
      tonic-popover .tonic--overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        opacity: 0;
        display: none;
        z-index: 0;
        background-color: rgba(0,0,0,0);
      }

      tonic-popover .tonic--popover {
        position: absolute;
        top: 30px;
        background: var(--tonic-window, #fff);
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 2px;
        visibility: hidden;
        z-index: -1;
        opacity: 0;
        -webkit-transform: scale(0.75);
        ms-transform: scale(0.75);
        transform: scale(0.75);
        transition: transform 0.1s ease-in-out, opacity 0s ease 0.1s, visibility 0s ease 0.1s, z-index 0s ease 0.1s;
      }

      tonic-popover .tonic--popover.tonic--show {
        box-shadow: 0px 30px 90px -20px rgba(0, 0, 0, 0.3);
        -webkit-transform: scale(1);
        -ms-transform: scale(1);
        transform: scale(1);
        visibility: visible;
        transition: transform 0.15s ease-in-out;
        opacity: 1;
        z-index: 1;
      }

      tonic-popover .tonic--show ~ .tonic--overlay {
        display: block;
        opacity: 1;
      }

      tonic-popover .tonic--popover--top {
        transform-origin: bottom center;
      }

      tonic-popover .tonic--popover--topleft {
        transform-origin: bottom left;
      }

      tonic-popover .tonic--popover--topright {
        transform-origin: bottom right;
      }

      tonic-popover .tonic--popover--bottom {
        transform-origin: top center;
      }

      tonic-popover .tonic--popover--bottomleft {
        transform-origin: top left;
      }

      tonic-popover .tonic--popover--bottomright {
        transform-origin: top right;
      }

    `
  }

  styles () {
    const {
      width,
      height,
      padding,
      margin,
      position
    } = this.props

    return {
      popover: {
        width,
        height,
        padding,
        margin,
        position
      }
    }
  }

  show (triggerNode) {
    const popover = this.querySelector('.tonic--popover')
    let scrollableArea = triggerNode.parentNode

    while (true) {
      if (!scrollableArea || scrollableArea.tagName === 'BODY') break
      if (window.getComputedStyle(scrollableArea).overflow === 'scroll') break
      scrollableArea = scrollableArea.parentNode
    }

    const margin = parseInt(this.props.margin, 10)
    let { top, left } = triggerNode.getBoundingClientRect()
    let pos = top + scrollableArea.scrollTop
    left -= scrollableArea.offsetLeft

    switch (this.props.position) {
      case 'topleft':
        pos -= popover.offsetHeight + margin
        break
      case 'topright':
        pos -= popover.offsetHeight + margin
        left += (triggerNode.offsetWidth - popover.offsetWidth)
        break
      case 'top':
        pos -= popover.offsetHeight + margin
        left += (triggerNode.offsetWidth / 2) - (popover.offsetWidth / 2)
        break
      case 'bottomleft':
        pos += triggerNode.offsetHeight + margin
        break
      case 'bottomright':
        pos += triggerNode.offsetHeight + margin
        left += triggerNode.offsetWidth - popover.offsetWidth
        break
      case 'bottom':
        pos += triggerNode.offsetHeight + margin
        left += (triggerNode.offsetWidth / 2) - (popover.offsetWidth / 2)
        break
    }

    popover.style.top = `${pos}px`
    popover.style.left = `${left}px`

    window.requestAnimationFrame(() => {
      popover.className = `tonic--popover tonic--show tonic--popover--${this.props.position}`
      const event = new window.Event('show')
      this.dispatchEvent(event)
    })
  }

  hide () {
    const popover = this.querySelector('.tonic--popover')
    if (popover) popover.classList.remove('tonic--show')
  }

  connected () {
    if (!this.props.open) return
    const target = this.getAttribute('for')
    this.show(document.getElementById(target))
  }

  click (e) {
    if (Tonic.match(e.target, '.tonic--overlay')) {
      return this.hide()
    }
  }

  render () {
    const {
      theme
    } = this.props

    if (theme) this.classList.add(`tonic--theme--${theme}`)

    return this.html`
      <div class="tonic--popover" styles="popover">
        ${this.nodes}
      </div>
      <div class="tonic--overlay"></div>
    `
  }
}

module.exports = { TonicPopover }

},{"@optoolco/tonic":12}],16:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicProfileImage extends Tonic {
  get value () {
    const state = this.state
    return state.data || this.props.src
  }

  defaults () {
    return {
      src: TonicProfileImage.svg.default(),
      size: '50px',
      radius: '5px'
    }
  }

  static stylesheet () {
    return `
      tonic-profile-image {
        display: inline-block;
      }

      tonic-profile-image .tonic--wrapper {
        position: relative;
        overflow: hidden;
      }

      tonic-profile-image .tonic--image {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-size: cover;
        background-position: center center;
        background-repeat: no-repeat;
      }

      tonic-profile-image .tonic--overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0,0,0,0.5);
        transition: opacity 0.2s ease-in-out;
        visibility: hidden;
        opacity: 0;
        display: flex;
      }

      tonic-profile-image .tonic--overlay .tonic--icon {
        width: 30px;
        height: 30px;
        position: absolute;
        top: 50%;
        left: 50%;
        -webkit-transform: translateX(-50%) translateY(-50%);
        -moz-transform: translateX(-50%) translateY(-50%);
        transform: translateX(-50%) translateY(-50%);
      }

      tonic-profile-image .tonic--overlay .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-profile-image .tonic--wrapper.tonic--editable:hover .tonic--overlay {
        visibility: visible;
        opacity: 1;
        cursor: pointer;
      }
    `
  }

  styles () {
    const {
      src,
      size,
      border,
      radius
    } = this.props

    return {
      background: {
        backgroundImage: `url('${src}')`
      },
      hidden: {
        display: 'none'
      },
      wrapper: {
        width: size,
        height: size,
        border: border,
        borderRadius: radius
      }
    }
  }

  getPictureData (src, cb) {
    const reader = new window.FileReader()
    reader.onerror = err => cb(err)
    reader.onload = e => cb(null, e.target.result)
    reader.readAsDataURL(src)
  }

  click (e) {
    if (this.props.editable) {
      if (this.props.editable === 'false') return
      const fileInput = this.getElementsByTagName('input')[0]
      fileInput.click()
    }
  }

  change (e) {
    const fileInput = this.getElementsByTagName('input')[0]
    const data = fileInput.files[0]
    if (e.data) return
    e.stopPropagation()

    const {
      size,
      type,
      path,
      lastModifiedDate
    } = data

    this.getPictureData(data, (err, data) => {
      if (err) {
        const event = new window.Event('error')
        event.message = err.message
        this.dispatchEvent(event)
        return
      }

      const slot = this.querySelector('.tonic--image')

      this.state.size = size
      this.state.path = path
      this.state.time = type
      this.state.mtime = lastModifiedDate
      this.state.data = data

      slot.style.backgroundImage = 'url("' + data + '")'
      const event = new window.Event('change', { bubbles: true })
      event.data = true // prevent recursion
      this.dispatchEvent(event)
    })
  }

  render () {
    const {
      theme,
      editable
    } = this.props

    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const classes = ['tonic--wrapper']
    if (editable === 'true') classes.push('tonic--editable')

    return this.html`
      <div
        class="${classes.join(' ')}"
        styles="wrapper">

        <div
          class="tonic--image"
          styles="background">
        </div>

        <input type="file" styles="hidden"/>

        <div class="tonic--overlay">
          <div class="tonic--icon">
            <svg>
              <use
                href="#edit"
                xlink:href="#edit"
                color="#fff"
                fill="#fff">
              </use>
            </svg>
          </div>
        </div>
      </div>
    `
  }
}

TonicProfileImage.svg = {}
TonicProfileImage.svg.toURL = s => `data:image/svg+xml;base64,${window.btoa(s)}`
TonicProfileImage.svg.default = () => TonicProfileImage.svg.toURL(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
    <rect fill="#F0F0F0" width="100" height="100"></rect>
    <circle fill="#D6D6D6" cx="49.3" cy="41.3" r="21.1"></circle>
    <path fill="#D6D6D6" d="M48.6,69.5c-18.1,0-33.1,13.2-36,30.5h72C81.8,82.7,66.7,69.5,48.6,69.5z"></path>
  </svg>
`)

module.exports = { TonicProfileImage }

},{"@optoolco/tonic":12}],17:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicProgressBar extends Tonic {
  set value (value) {
    this.setProgress(value)
  }

  get value () {
    if (typeof this.state.progress !== 'undefined') {
      return this.state.progress
    }

    return this.props.progress
  }

  defaults () {
    return {
      width: '280px',
      height: '15px',
      progress: 0
    }
  }

  static stylesheet () {
    return `
      tonic-progress-bar {
        display: inline-block;
        user-select: none;
      }

      tonic-progress-bar .tonic--wrapper {
        position: relative;
        background-color: var(--tonic-background, #fff);
      }

      tonic-progress-bar .tonic--wrapper .tonic--progress {
        background-color: var(--tonic-accent, #f66);
        width: 0%;
        height: 100%;
      }
    `
  }

  styles () {
    const progress = this.state.progress || this.props.progress
    return {
      wrapper: {
        width: this.props.width,
        height: this.props.height
      },
      progress: {
        width: progress + '%',
        backgroundColor: this.props.color || 'var(--tonic-accent, #f66)'
      }
    }
  }

  setProgress (progress) {
    this.state.progress = progress
    this.reRender()
  }

  render () {
    if (this.props.theme) {
      this.classList.add(`tonic--theme--${this.props.theme}`)
    }

    this.style.width = this.props.width
    this.style.height = this.props.height

    return this.html`
      <div class="tonic--wrapper" styles="wrapper">
        <div class="tonic--progress" styles="progress"></div>
      </div>
    `
  }
}

module.exports = { TonicProgressBar }

},{"@optoolco/tonic":12}],18:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicRange extends Tonic {
  defaults () {
    return {
      width: '250px',
      disabled: false,
      min: '0',
      max: '100',
      value: '50'
    }
  }

  get value () {
    return this.state.value
  }

  set value (value) {
    this.querySelector('input').value = value
    this.setValue(value)
  }

  setValue (value) {
    const min = this.props.min
    const max = this.props.max

    const input = this.querySelector('input')

    if (this.props.label) {
      const label = this.querySelector('label')
      label.textContent = this.getLabelValue(value)
    }

    input.style.backgroundSize = (value - min) * 100 / (max - min) + '% 100%'

    this.state.value = value
  }

  input (e) {
    this.setValue(e.target.value || this.props.value)
  }

  static stylesheet () {
    return `
      tonic-range  {
        position: relative;
        display: -webkit-flex;
        display: flex;
        height: 50px;
        padding: 20px 0;
      }

      tonic-range.tonic--no-label {
        height: 30px;
      }

      tonic-range label {
        font: 13px var(--tonic-subheader, 'Arial', sans-serif);
        letter-spacing: 1px;
        text-align: center;
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
      }

      tonic-range input[type="range"] {
        margin: auto;
        padding: 0;
        width: 50%;
        height: 4px;
        background-color: var(--tonic-secondary, #fff);
        background-image: -webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, var(--tonic-accent, #f66)), color-stop(100%, var(--tonic-accent, #f66)));
        background-image: -webkit-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: -moz-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: -o-linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-image: linear-gradient(var(--tonic-accent, #f66), var(--tonic-accent, #f66));
        background-size: 50% 100%;
        background-repeat: no-repeat;
        border-radius: 0;
        cursor: pointer;
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]:focus {
        outline-offset: 4px;
      }

      tonic-range input[type="range"]:disabled {
        background-image: -webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, var(--tonic-border, #ccc)), color-stop(100%, var(--tonic-border, #ccc)));
        background-image: -webkit-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: -moz-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: -o-linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
        background-image: linear-gradient(var(--tonic-border, #ccc), var(--tonic-border, #ccc));
      }

      tonic-range input[type="range"]::-webkit-slider-runnable-track {
        box-shadow: none;
        border: none;
        background: transparent;
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]::-moz-range-track {
        box-shadow: none;
        border: none;
        background: transparent;
      }

      tonic-range input[type="range"]::-moz-focus-outer {
        border: 0;
      }

      tonic-range input[type="range"]::-webkit-slider-thumb {
        width: 18px;
        height: 18px;
        border: 0;
        background: var(--tonic-border);
        border-radius: 100%;
        box-shadow: 0 0 3px 0px rgba(0,0,0,0.25);
        -webkit-appearance: none;
      }

      tonic-range input[type="range"]::-moz-range-thumb {
        width: 18px;
        height: 18px;
        border: 0;
        background: var(--tonic-border);
        border-radius: 100%;
        box-shadow: 0 0 1px 0px rgba(0,0,0,0.1);
      }
    `
  }

  getLabelValue (value) {
    if (this.setLabel) {
      return this.setLabel(value)
    } else if (this.props.label) {
      return this.props.label.replace(/%\w/, value)
    } else {
      return value
    }
  }

  renderLabel () {
    if (!this.props.label) return ''
    const value = this.props.value
    return this.html`<label>${this.getLabelValue(value)}</label>`
  }

  styles () {
    const {
      width
    } = this.props

    return {
      width: {
        width
      }
    }
  }

  connected () {
    this.setValue(this.state.value)
  }

  render () {
    const {
      width,
      height,
      disabled,
      theme,
      min,
      max,
      step,
      label,
      id,
      tabindex
    } = this.props

    if (width) this.style.width = width
    if (height) this.style.width = height
    if (theme) this.classList.add(`tonic--theme--${theme}`)
    if (!label) this.classList.add('tonic--no-label')
    if (tabindex) this.removeAttribute('tabindex')

    const value = this.props.value || this.state.value
    if (typeof this.state.value === 'undefined') {
      this.state.value = value
    }

    return this.html`
      ${this.renderLabel()}
      <div class="tonic--wrapper" styles="width">
        <input ... ${{
          type: 'range',
          styles: 'width',
          id,
          value,
          tabindex,
          step,
          min,
          max,
          disabled: disabled === 'true'
        }}/>
      </div>
    `
  }
}

module.exports = { TonicRange }

},{"@optoolco/tonic":12}],19:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

const weekdays = [
  'Sunday',
  'Monday',
  'Tuesday',
  'Wednesday',
  'Thursday',
  'Friday',
  'Saturday'
]

const months = [
  'January',
  'February',
  'March',
  'April',
  'May',
  'June',
  'July',
  'August',
  'September',
  'October',
  'November',
  'December'
]

function pad (num) {
  return `0${num}`.slice(-2)
}

function strftime (time, formatString) {
  const day = time.getDay()
  const date = time.getDate()
  const month = time.getMonth()
  const year = time.getFullYear()
  const hour = time.getHours()
  const minute = time.getMinutes()
  const second = time.getSeconds()

  return formatString.replace(/%([%aAbBcdeHIlmMpPSwyYZz])/g, (_arg) => {
    let match
    const modifier = _arg[1]

    switch (modifier) {
      case '%':
        return '%'
      case 'a':
        return weekdays[day].slice(0, 3)
      case 'A':
        return weekdays[day]
      case 'b':
        return months[month].slice(0, 3)
      case 'B':
        return months[month]
      case 'c':
        return time.toString()
      case 'd':
        return pad(date)
      case 'e':
        return String(date)
      case 'H':
        return pad(hour)
      case 'I':
        return pad(strftime(time, '%l'))
      case 'l':
        if (hour === 0 || hour === 12) {
          return String(12)
        } else {
          return String((hour + 12) % 12)
        }
      case 'm':
        return pad(month + 1)
      case 'M':
        return pad(minute)
      case 'p':
        if (hour > 11) {
          return 'PM'
        } else {
          return 'AM'
        }
      case 'P':
        if (hour > 11) {
          return 'pm'
        } else {
          return 'am'
        }
      case 'S':
        return pad(second)
      case 'w':
        return String(day)
      case 'y':
        return pad(year % 100)
      case 'Y':
        return String(year)
      case 'Z':
        match = time.toString().match(/\((\w+)\)$/)
        return match ? match[1] : ''
      case 'z':
        match = time.toString().match(/\w([+-]\d\d\d\d) /)
        return match ? match[1] : ''
    }
    return ''
  })
}

function makeFormatter (options) {
  let format

  return function () {
    if (format) {
      return format
    }

    if ('Intl' in window) {
      try {
        format = new Intl.DateTimeFormat(undefined, options)
        return format
      } catch (e) {
        if (!(e instanceof RangeError)) {
          throw e
        }
      }
    }
  }
}

let dayFirst = null
const dayFirstFormatter = makeFormatter({ day: 'numeric', month: 'short' })

// Private: Determine if the day should be formatted before the month name in
// the user's current locale. For example, `9 Jun` for en-GB and `Jun 9`
// for en-US.
//
// Returns true if the day appears before the month.
function isDayFirst () {
  if (dayFirst !== null) {
    return dayFirst
  }

  const formatter = dayFirstFormatter()
  if (formatter) {
    const output = formatter.format(new Date(0))
    dayFirst = !!output.match(/^\d/)
    return dayFirst
  } else {
    return false
  }
}

let yearSeparator = null

const yearFormatter = makeFormatter({
  day: 'numeric',
  month: 'short',
  year: 'numeric'
})

// Private: Determine if the year should be separated from the month and day
// with a comma. For example, `9 Jun 2014` in en-GB and `Jun 9, 2014` in en-US.
//
// Returns true if the date needs a separator.
function isYearSeparator () {
  if (yearSeparator !== null) {
    return yearSeparator
  }

  const formatter = yearFormatter()

  if (formatter) {
    const output = formatter.format(new Date(0))
    yearSeparator = !!output.match(/\d,/)
    return yearSeparator
  } else {
    return true
  }
}

function isThisYear (date) {
  const now = new Date()
  return now.getUTCFullYear() === date.getUTCFullYear()
}

function makeRelativeFormat (locale, options) {
  if ('Intl' in window && 'RelativeTimeFormat' in window.Intl) {
    try {
      return new Intl.RelativeTimeFormat(locale, options)
    } catch (e) {
      if (!(e instanceof RangeError)) {
        throw e
      }
    }
  }
}

function localeFromElement (el) {
  const container = el.closest('[lang]')

  if (container instanceof window.HTMLElement && container.lang) {
    return container.lang
  }

  return 'default'
}

class RelativeTime {
  constructor (date, locale) {
    this.date = date
    this.locale = locale
  }

  toString () {
    const ago = this.timeElapsed()

    if (ago) {
      return ago
    }

    const ahead = this.timeAhead()

    if (ahead) {
      return ahead
    } else {
      return `on ${this.formatDate()}`
    }
  }

  get value () {
    return this.date
  }

  timeElapsed ({ date = this.date, locale = this.locale } = {}) {
    const ms = new Date().getTime() - date.getTime()
    const sec = Math.round(ms / 1000)
    const min = Math.round(sec / 60)
    const hr = Math.round(min / 60)
    const day = Math.round(hr / 24)

    if (ms >= 0 && day < 30) {
      return this.timeAgoFromMs({ ms, date, locale })
    } else {
      return null
    }
  }

  timeAhead ({ date = this.date, locale = this.locale } = {}) {
    const ms = date.getTime() - new Date().getTime()
    const sec = Math.round(ms / 1000)
    const min = Math.round(sec / 60)
    const hr = Math.round(min / 60)
    const day = Math.round(hr / 24)
    if (ms >= 0 && day < 30) {
      return this.timeUntil({ date, locale })
    } else {
      return null
    }
  }

  timeAgo ({ date = this.date, locale = this.locale } = {}) {
    const ms = new Date().getTime() - date.getTime()
    return this.timeAgoFromMs({ ms, date, locale })
  }

  timeAgoFromMs ({ ms, locale = this.locale } = {}) {
    const sec = Math.round(ms / 1000)
    const min = Math.round(sec / 60)
    const hr = Math.round(min / 60)
    const day = Math.round(hr / 24)
    const month = Math.round(day / 30)
    const year = Math.round(month / 12)

    if (ms < 0) {
      return formatRelativeTime(locale, 0, 'second')
    } else if (sec < 10) {
      return formatRelativeTime(locale, 0, 'second')
    } else if (sec < 45) {
      return formatRelativeTime(locale, -sec, 'second')
    } else if (sec < 90) {
      return formatRelativeTime(locale, -min, 'minute')
    } else if (min < 45) {
      return formatRelativeTime(locale, -min, 'minute')
    } else if (min < 90) {
      return formatRelativeTime(locale, -hr, 'hour')
    } else if (hr < 24) {
      return formatRelativeTime(locale, -hr, 'hour')
    } else if (hr < 36) {
      return formatRelativeTime(locale, -day, 'day')
    } else if (day < 30) {
      return formatRelativeTime(locale, -day, 'day')
    } else if (month < 12) {
      return formatRelativeTime(locale, -month, 'month')
    } else if (month < 18) {
      return formatRelativeTime(locale, -year, 'year')
    } else {
      return formatRelativeTime(locale, -year, 'year')
    }
  }

  timeUntil ({ date = this.date, locale = this.locale } = {}) {
    const ms = date.getTime() - new Date().getTime()
    return this.timeUntilFromMs({ ms, locale })
  }

  timeUntilFromMs ({ ms, locale = this.locale } = {}) {
    const sec = Math.round(ms / 1000)
    const min = Math.round(sec / 60)
    const hr = Math.round(min / 60)
    const day = Math.round(hr / 24)
    const month = Math.round(day / 30)
    const year = Math.round(month / 12)

    if (month >= 18) {
      return formatRelativeTime(locale, year, 'year')
    } else if (month >= 12) {
      return formatRelativeTime(locale, year, 'year')
    } else if (day >= 45) {
      return formatRelativeTime(locale, month, 'month')
    } else if (day >= 30) {
      return formatRelativeTime(locale, month, 'month')
    } else if (hr >= 36) {
      return formatRelativeTime(locale, day, 'day')
    } else if (hr >= 24) {
      return formatRelativeTime(locale, day, 'day')
    } else if (min >= 90) {
      return formatRelativeTime(locale, hr, 'hour')
    } else if (min >= 45) {
      return formatRelativeTime(locale, hr, 'hour')
    } else if (sec >= 90) {
      return formatRelativeTime(locale, min, 'minute')
    } else if (sec >= 45) {
      return formatRelativeTime(locale, min, 'minute')
    } else if (sec >= 10) {
      return formatRelativeTime(locale, sec, 'second')
    } else {
      return formatRelativeTime(locale, 0, 'second')
    }
  }

  formatDate ({ date = this.date } = {}) {
    let format = isDayFirst() ? '%e %b' : '%b %e'

    if (!isThisYear(date)) {
      format += isYearSeparator() ? ', %Y' : ' %Y'
    }

    return strftime(date, format)
  }

  formatTime ({ date = this.date } = {}) {
    const formatter = timeFormatter()

    if (formatter) {
      return formatter.format(date)
    } else {
      return strftime(date, '%l:%M%P')
    }
  }
}

function formatRelativeTime (locale, value, unit) {
  const formatter = makeRelativeFormat(locale, { numeric: 'auto' })

  if (formatter) {
    return formatter.format(value, unit)
  } else {
    return formatEnRelativeTime(value, unit)
  }
}

// Simplified "en" RelativeTimeFormat.format function
//
// Values should roughly match
//   new Intl.RelativeTimeFormat('en', {numeric: 'auto'}).format(value, unit)
//
function formatEnRelativeTime (value, unit) {
  if (value === 0) {
    switch (unit) {
      case 'year':
      case 'quarter':
      case 'month':
      case 'week':
        return `this ${unit}`
      case 'day':
        return 'today'
      case 'hour':
      case 'minute':
        return `in 0 ${unit}s`
      case 'second':
        return 'now'
    }
  } else if (value === 1) {
    switch (unit) {
      case 'year':
      case 'quarter':
      case 'month':
      case 'week':
        return `next ${unit}`
      case 'day':
        return 'tomorrow'
      case 'hour':
      case 'minute':
      case 'second':
        return `in 1 ${unit}`
    }
  } else if (value === -1) {
    switch (unit) {
      case 'year':
      case 'quarter':
      case 'month':
      case 'week':
        return `last ${unit}`
      case 'day':
        return 'yesterday'
      case 'hour':
      case 'minute':
      case 'second':
        return `1 ${unit} ago`
    }
  } else if (value > 1) {
    switch (unit) {
      case 'year':
      case 'quarter':
      case 'month':
      case 'week':
      case 'day':
      case 'hour':
      case 'minute':
      case 'second':
        return `in ${value} ${unit}s`
    }
  } else if (value < -1) {
    switch (unit) {
      case 'year':
      case 'quarter':
      case 'month':
      case 'week':
      case 'day':
      case 'hour':
      case 'minute':
      case 'second':
        return `${-value} ${unit}s ago`
    }
  }

  throw new RangeError(`Invalid unit argument for format() '${unit}'`)
}

class TonicRelativeTime extends Tonic {
  render () {
    let date = this.props.date || ''
    const locale = this.props.locale || localeFromElement(this)

    if (typeof date === 'string') {
      date = this.props.date = new Date(this.props.date)
    }

    if (date.toString() === 'Invalid Date') {
      date = new Date()
    }

    if (this.props.refresh) {
      this.interval = setInterval(() => {
        this.reRender(props => ({
          ...props,
          date
        }))
      }, +this.props.refresh)
    }

    const t = new RelativeTime(date, locale)
    return t.toString()
  }
}

const timeFormatter = makeFormatter({
  hour: 'numeric',
  minute: '2-digit'
})

module.exports = { TonicRelativeTime, RelativeTime }

},{"@optoolco/tonic":12}],20:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicRouter extends Tonic {
  constructor () {
    super()

    if (TonicRouter.patched) return
    TonicRouter.patched = true

    const patchEvent = function (type) {
      const orig = window.history[type]
      return function (...args) {
        const value = orig.call(this, ...args)

        window.dispatchEvent(new window.Event(type.toLowerCase()))
        TonicRouter.route()

        return value
      }
    }

    window.addEventListener('popstate', e => TonicRouter.route())
    window.history.pushState = patchEvent('pushState')
    window.history.replaceState = patchEvent('replaceState')
  }

  static route (routes, reset) {
    routes = routes || [...document.getElementsByTagName('tonic-router')]
    const keys = []
    let defaultRoute = null
    let hasMatch = false
    TonicRouter.props = {}

    for (const route of routes) {
      const path = route.getAttribute('path')

      route.removeAttribute('match')

      if (!path) {
        defaultRoute = route
        defaultRoute.reRender && defaultRoute.reRender()
        continue
      }

      const matcher = TonicRouter.matcher(path, keys)
      const match = matcher.exec(window.location.pathname)

      if (match) {
        route.setAttribute('match', true)
        hasMatch = true

        match.slice(1).forEach((m, i) => {
          TonicRouter.props[keys[i].name] = m
        })
      } else {
        route.removeAttribute('match')
      }

      if (!reset) {
        route.reRender && route.reRender()
      }
    }

    if (!reset && !hasMatch && defaultRoute) {
      defaultRoute.setAttribute('match', true)
      defaultRoute.reRender && defaultRoute.reRender()
    }
  }

  willConnect () {
    const attrId = this.getAttribute('id')
    this.id = attrId || this.getAttribute('path')
    this.template = document.createElement('template')
    this.template.innerHTML = this.innerHTML
    TonicRouter.route([this], true)
  }

  updated () {
    if (this.hasAttribute('match')) {
      this.dispatchEvent(new window.Event('match'))
    }
  }

  render () {
    if (this.hasAttribute('match')) {
      this.state = TonicRouter.props
      return this.template.content
    }

    return ''
  }
}

TonicRouter.matcher = (() => {
  //
  // Most of this was lifted from the path-to-regex project which can
  // be found here -> https://github.com/pillarjs/path-to-regexp
  //
  const DEFAULT_DELIMITER = '/'
  const DEFAULT_DELIMITERS = './'

  const PATH_REGEXP = new RegExp([
    '(\\\\.)',
    '(?:\\:(\\w+)(?:\\(((?:\\\\.|[^\\\\()])+)\\))?|\\(((?:\\\\.|[^\\\\()])+)\\))([+*?])?'
  ].join('|'), 'g')

  function parse (str, options) {
    const tokens = []
    let key = 0
    let index = 0
    let path = ''
    const defaultDelimiter = (options && options.delimiter) || DEFAULT_DELIMITER
    const delimiters = (options && options.delimiters) || DEFAULT_DELIMITERS
    let pathEscaped = false
    let res

    while ((res = PATH_REGEXP.exec(str)) !== null) {
      const m = res[0]
      const escaped = res[1]
      const offset = res.index
      path += str.slice(index, offset)
      index = offset + m.length

      // Ignore already escaped sequences.
      if (escaped) {
        path += escaped[1]
        pathEscaped = true
        continue
      }

      let prev = ''
      const next = str[index]
      const name = res[2]
      const capture = res[3]
      const group = res[4]
      const modifier = res[5]

      if (!pathEscaped && path.length) {
        const k = path.length - 1

        if (delimiters.indexOf(path[k]) > -1) {
          prev = path[k]
          path = path.slice(0, k)
        }
      }

      if (path) {
        tokens.push(path)
        path = ''
        pathEscaped = false
      }

      const partial = prev !== '' && next !== undefined && next !== prev
      const repeat = modifier === '+' || modifier === '*'
      const optional = modifier === '?' || modifier === '*'
      const delimiter = prev || defaultDelimiter
      const pattern = capture || group

      tokens.push({
        name: name || key++,
        prefix: prev,
        delimiter: delimiter,
        optional: optional,
        repeat: repeat,
        partial: partial,
        pattern: pattern ? escapeGroup(pattern) : '[^' + escapeString(delimiter) + ']+?'
      })
    }

    if (path || index < str.length) {
      tokens.push(path + str.substr(index))
    }

    return tokens
  }

  function escapeString (str) {
    return str.replace(/([.+*?=^!:${}()[\]|/\\])/g, '\\$1')
  }

  function escapeGroup (group) {
    return group.replace(/([=!:$/()])/g, '\\$1')
  }

  function flags (options) {
    return options && options.sensitive ? '' : 'i'
  }

  function regexpToRegexp (path, keys) {
    if (!keys) return path

    const groups = path.source.match(/\((?!\?)/g)

    if (groups) {
      for (let i = 0; i < groups.length; i++) {
        keys.push({
          name: i,
          prefix: null,
          delimiter: null,
          optional: false,
          repeat: false,
          partial: false,
          pattern: null
        })
      }
    }

    return path
  }

  function arrayToRegexp (path, keys, options) {
    const parts = []

    for (let i = 0; i < path.length; i++) {
      parts.push(pathToRegexp(path[i], keys, options).source)
    }

    return new RegExp('(?:' + parts.join('|') + ')', flags(options))
  }

  function stringToRegexp (path, keys, options) {
    return tokensToRegExp(parse(path, options), keys, options)
  }

  function tokensToRegExp (tokens, keys, options) {
    options = options || {}

    const strict = options.strict
    const end = options.end !== false
    const delimiter = escapeString(options.delimiter || DEFAULT_DELIMITER)
    const delimiters = options.delimiters || DEFAULT_DELIMITERS
    const endsWith = [].concat(options.endsWith || []).map(escapeString).concat('$').join('|')
    let route = ''
    let isEndDelimited = tokens.length === 0

    for (let i = 0; i < tokens.length; i++) {
      const token = tokens[i]

      if (typeof token === 'string') {
        route += escapeString(token)
        isEndDelimited = i === tokens.length - 1 && delimiters.indexOf(token[token.length - 1]) > -1
      } else {
        const prefix = escapeString(token.prefix)
        const capture = token.repeat
          ? '(?:' + token.pattern + ')(?:' + prefix + '(?:' + token.pattern + '))*'
          : token.pattern

        if (keys) keys.push(token)

        if (token.optional) {
          if (token.partial) {
            route += prefix + '(' + capture + ')?'
          } else {
            route += '(?:' + prefix + '(' + capture + '))?'
          }
        } else {
          route += prefix + '(' + capture + ')'
        }
      }
    }

    if (end) {
      if (!strict) route += '(?:' + delimiter + ')?'

      route += endsWith === '$' ? '$' : '(?=' + endsWith + ')'
    } else {
      if (!strict) route += '(?:' + delimiter + '(?=' + endsWith + '))?'
      if (!isEndDelimited) route += '(?=' + delimiter + '|' + endsWith + ')'
    }

    return new RegExp('^' + route, flags(options))
  }

  function pathToRegexp (path, keys, options) {
    if (path instanceof RegExp) {
      return regexpToRegexp(path, keys)
    }

    if (Array.isArray(path)) {
      return arrayToRegexp(/** @type {!Array} */ (path), keys, options)
    }

    return stringToRegexp(/** @type {string} */ (path), keys, options)
  }

  return pathToRegexp
})()

module.exports = { TonicRouter }

},{"@optoolco/tonic":12}],21:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicSelect extends Tonic {
  defaults () {
    return {
      disabled: false,
      invalid: false,
      iconArrow: TonicSelect.svg.default(),
      width: '250px',
      radius: '2px'
    }
  }

  static stylesheet () {
    return `
      tonic-select .tonic--wrapper {
        position: relative;
      }

      tonic-select .tonic--wrapper:before {
        content: '';
        width: 14px;
        height: 14px;
        opacity: 0;
        z-index: 1;
      }

      tonic-select.tonic--loading {
        pointer-events: none;
        transition: background 0.3s ease;
      }

      tonic-select.tonic--loading select {
        color: transparent;
        background-color: var(--tonic-window, #fff);
        border-color: var(--tonic-border, #ccc);
      }

      tonic-select.tonic--loading .tonic--wrapper:before {
        margin-top: -8px;
        margin-left: -8px;
        display: block;
        position: absolute;
        bottom: 10px;
        left: 50%;
        opacity: 1;
        transform: translateX(-50%);
        border: 2px solid var(--tonic-medium, #999);
        border-radius: 50%;
        border-top-color: transparent;
        animation: spin 1s linear 0s infinite;
        transition: opacity 0.3s ease;
      }

      tonic-select select {
        color: var(--tonic-primary, #333);
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        background-color: var(--tonic-input-background, var(--tonic-background, #f66));
        background-repeat: no-repeat;
        background-position: center right;
        border: 1px solid var(--tonic-border, #ccc);
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
        position: relative;
      }

      tonic-select select:focus {
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
        outline: none;
      }

      tonic-select[edited] select[invalid],
      tonic-select[edited] select:invalid,
      tonic-select[edited] select[invalid]:focus,
      tonic-select[edited] select:invalid:focus {
        border-color: var(--tonic-error, #f66);
      }

      tonic-select[edited] select[invalid] ~ .tonic--invalid,
      tonic-select[edited] select:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-select[label] .tonic--invalid {
        margin-bottom: -13px;
      }

      tonic-select .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
      }

      tonic-select .tonic--invalid span {
        color: white;
        padding: 2px 6px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-select .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }

      tonic-select select:not([multiple]) {
        padding: 8px 30px 8px 8px;
      }

      tonic-select select[disabled] {
        background-color: transparent;
      }

      tonic-select label {
        color: var(--tonic-medium, #999);
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        font-weight: 500;
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
      }

      tonic-select[multiple] select {
        background-image: none !important;
      }

      tonic-select[multiple] select option {
        padding: 6px 8px;
      }

      @keyframes spin {
        from {
          transform: rotate(0deg);
        }
        to {
          transform: rotate(360deg);
        }
      }
    `
  }

  setValid () {
    const input = this.querySelector('select')
    if (!input) return

    input.setCustomValidity('')
    input.removeAttribute('invalid')
  }

  setInvalid (msg) {
    const input = this.querySelector('select')
    if (!input) return

    this.setAttribute('edited', true)

    msg = msg || this.props.errorMessage

    input.setCustomValidity(msg)
    input.setAttribute('invalid', msg)
    const span = this.querySelector('.tonic--invalid span')
    if (!span) return

    span.textContent = msg
  }

  get value () {
    const el = this.querySelector('select')

    if (this.props.multiple === 'true') {
      const value = [...el.options]
        .filter(el => el.selected)
        .map(el => el.getAttribute('value'))
      return value
    }

    return el.value
  }

  selectOptions (value) {
    const el = this.querySelector('select')
    const options = [...el.options]

    options.forEach(el => {
      el.selected = value.findIndex(v => v === el.value) > -1
    })
  }

  set value (value) {
    const multiSelect = (this.props.multiple === 'true') && Array.isArray(value)
    const el = this.root.querySelector('select')

    if (multiSelect) {
      this.selectOptions(value)
    } else if (!value) {
      value = el.selectedIndex
      el.selectedIndex = 0
    } else {
      el.value = value
    }
  }

  get option () {
    const node = this.querySelector('select')
    return node.options[node.selectedIndex]
  }

  get selectedIndex () {
    const node = this.querySelector('select')
    return node.selectedIndex
  }

  set selectedIndex (index) {
    const node = this.querySelector('select')
    node.selectedIndex = index
  }

  loading (state) {
    const method = state ? 'add' : 'remove'
    this.classList[method]('tonic--loading')
  }

  renderLabel () {
    if (!this.props.label) return ''
    return this.html`<label>${this.props.label}</label>`
  }

  styles () {
    const {
      height,
      width,
      padding,
      radius,
      iconArrow
    } = this.props

    return {
      wrapper: {
        width
      },
      select: {
        width,
        height,
        borderRadius: radius,
        padding,
        backgroundImage: `url('${iconArrow}')`
      }
    }
  }

  setupEvents () {
    const input = this.querySelector('select')
    input.addEventListener('change', _ => {
      this.setAttribute('edited', true)
    })
  }

  updated () {
    this.setupEvents()
  }

  connected () {
    const value = this.props.value

    if (Array.isArray(value)) {
      this.selectOptions(value)
    } else if (value) {
      const option = this.querySelector(`option[value="${value}"]`)
      if (option) option.setAttribute('selected', true)
    }
  }

  render () {
    const {
      width,
      height,
      disabled,
      required,
      multiple,
      theme,
      name,
      size,
      tabindex
    } = this.props

    if (width) this.style.width = width
    if (height) this.style.width = height
    if (theme) this.classList.add(`tonic--theme--${theme}`)
    if (name) this.removeAttribute('name')
    if (tabindex) this.removeAttribute('tabindex')

    const errorMessage = this.props.errorMessage ||
      this.props.errormessage || 'Invalid'

    return this.html`
      <div class="tonic--wrapper" styles="wrapper">
        ${this.renderLabel()}
        <select ... ${{
          styles: 'select',
          disabled: disabled === 'true',
          multiple: multiple === 'true',
          name,
          tabindex,
          required,
          size
        }}>
          ${this.childNodes}
        </select>

        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${errorMessage}</span>
        </div>
      </div>
    `
  }
}

TonicSelect.svg = {}
TonicSelect.svg.toURL = s => `data:image/svg+xml;base64,${window.btoa(s)}`
TonicSelect.svg.default = () => TonicSelect.svg.toURL(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
    <path fill="#D7DBDD" d="M61.4,45.8l-11,13.4c-0.2,0.3-0.5,0.3-0.7,0l-11-13.4c-0.3-0.3-0.1-0.8,0.4-0.8h22C61.4,45,61.7,45.5,61.4,45.8z"/>
  </svg>
`)

module.exports = { TonicSelect }

},{"@optoolco/tonic":12}],22:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicSplitLeft extends Tonic {
  render () {
    if (this.props.width) {
      this.style.width = this.props.width
    }

    return this.html`
      ${this.elements}
    `
  }
}

class TonicSplitTop extends Tonic {
  render () {
    if (this.props.height) {
      this.style.height = this.props.height
    }

    return this.html`
      ${this.elements}
    `
  }
}

class TonicSplitRight extends TonicSplitLeft {}
class TonicSplitBottom extends TonicSplitTop {}

class TonicSplit extends Tonic {
  constructor () {
    super()
    this.left = null
    this.right = null
    this.top = null
    this.bottom = null
    this.handleId = `tonic--handle-${Math.random().toString().slice(2)}`

    this.state.meta = {}
  }

  static stylesheet () {
    return `
      tonic-split {
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
      }

      tonic-split > tonic-split-top,
      tonic-split > tonic-split-bottom,
      tonic-split > tonic-split-left,
      tonic-split > tonic-split-right {
        position: absolute;
        overflow: hidden;
        will-change: width;
      }

      tonic-split > tonic-split-left,
      tonic-split > tonic-split-right {
        top: 0;
        bottom: 0;
      }

      tonic-split > tonic-split-left {
        left: 0;
        right: unset;
        width: 60%;
      }

      tonic-split > tonic-split-right {
        right: 0;
        left: unset;
        width: 40%;
      }

      tonic-split > tonic-split-top,
      tonic-split > tonic-split-bottom {
        left: 0;
        height: 50%;
        right: 0;
      }

      tonic-split > tonic-split-bottom {
        bottom: 0;
        top: unset;
      }

      tonic-split > tonic-split-top {
        top: 0;
        bottom: unset;
        z-index: 4;
      }

      tonic-split > tonic-split-right {
        right: 0;
        left: unset;
        width: 40%;
      }

      tonic-split .tonic--split-handle {
        position: absolute;
        z-index: 100;
        user-select: none;
        background-color: transparent;
        transition: background .1s ease;
      }

      #split-query > div {
        z-index: 101;
        margin-top: -5px;
      }

      tonic-split .tonic--split-vertical {
        top: 0;
        bottom: 0;
        left: 60%;
        width: 5px;
        border-left: 1px solid var(--tonic-border);
        cursor: ew-resize;
      }

      tonic-split .tonic--split-horizontal {
        left: 0;
        right: 0;
        height: 5px;
        top: 50%;
        border-bottom: 1px solid var(--tonic-border);
        cursor: ns-resize;
      }

      tonic-split .tonic--split-handle:hover {
        background: var(--tonic-accent);
      }

      tonic-split[dragging] tonic-split-right,
      tonic-split[dragging] tonic-split-left,
      tonic-split[dragging] tonic-split-top,
      tonic-split[dragging] tonic-split-bottom {
        pointer-events: none;
      }
    `
  }

  start () {
    this.dragging = true
    this.setAttribute('dragging', true)
  }

  cancel () {
    this.dragging = false
    this.removeAttribute('dragging')
  }

  willConnect () {
    this.updated()
  }

  toggle (panel, state) {
    const {
      meta
    } = this.state

    const previous = meta[panel]
    let opposite = ''
    let property = ''

    if (this.props.type === 'vertical') {
      opposite = panel === 'left' ? 'right' : 'left'
      property = 'width'
    } else {
      opposite = panel === 'top' ? 'bottom' : 'top'
      property = 'height'
    }

    if (!previous && !state) {
      //
      // First, save the state of the panel to hide and its opposite
      //
      meta[panel] = {
        [panel]: this[panel].style[property],
        [opposite]: this[opposite].style[property],
        handle: this.handle.style.display
      }

      //
      // Set the panel to hide as zero width, hide the handle
      // and set the opposite panel to fill the splitter
      //
      this[panel].style[property] = 0
      this[opposite].style[property] = '100%'
      this.handle.style.display = 'none'
      return
    }

    //
    // If there is meta data, use it to restore the previous
    // property values. After restored, delete the meta data.
    //
    if (previous) {
      this[panel].style[property] = previous[panel]
      this[opposite].style[property] = previous[opposite]
      this.handle.style.display = previous.handle
      delete meta[panel]
    }
  }

  connected () {
    this.handle = this.querySelector(`#${this.handleId}`)

    if (this.props.type === 'vertical') {
      this.left = this.elements[0]
      this.right = this.elements[1]
      this.handle.style.left = this.left.getAttribute('width')
    } else {
      this.top = this.elements[0]
      this.bottom = this.elements[1]
      this.handle.style.top = this.top.getAttribute('height')
    }
  }

  afterResize () {
    clearTimeout(this.timeout)
    this.timeout = setTimeout(() => {
      this.dispatchEvent(new window.CustomEvent(
        'resize', { bubbles: true }
      ))
    }, 64)
  }

  updated () {
    this.cancel()
  }

  disconnected () {
    this.handle = null
    this.left = null
    this.right = null
    this.top = null
    this.bottom = null
  }

  mousemove (e) {
    if (!this.dragging) return

    const { x, y } = this.getBoundingClientRect()

    const w = this.offsetWidth
    const h = this.offsetHeight

    const dirLeft = e.clientX >= this.lastX
    const dirDown = e.clientY >= this.lastY

    this.lastX = e.clientX + 1
    this.lastY = e.clientY - 1

    const max = parseInt(this.props.max, 10) || 25
    const min = parseInt(this.props.min, 10) || 25

    if (this.props.type === 'vertical') {
      if (dirLeft && this.right.offsetWidth <= max) {
        return this.cancel()
      }

      if (!dirLeft && this.left.offsetWidth <= min) {
        return this.cancel()
      }

      this.left = this.elements[0]
      this.right = this.elements[1]

      const t = e.clientX - x
      const p = (t / w) * 100

      this.left.style.width = p + '%'
      this.handle.style.left = p + '%'
      this.right.style.width = (100 - p) + '%'
      this.afterResize()
      return
    }

    if (!dirDown && this.top.offsetHeight <= min) {
      return this.cancel()
    }

    if (dirDown && this.bottom.offsetHeight <= max) {
      return this.cancel()
    }

    this.top = this.elements[0]
    this.bottom = this.elements[1]

    const t = e.clientY - y
    const p = (t / h) * 100

    if (p <= 100 - 5) {
      this.top.style.height = p + '%'
      this.handle.style.top = p + '%'
      this.bottom.style.height = (100 - p) + '%'
      this.afterResize()
    }
  }

  mouseenter (e) {
    this.cancel()
  }

  mouseleave (e) {
    this.cancel()
  }

  mousedown (e) {
    const handle = Tonic.match(e.target, '.tonic--split-handle')

    if (handle && handle.parentElement === this) {
      this.handle = handle
      this.start()
    }
  }

  mouseup (e) {
    this.cancel()
  }

  render () {
    const classes = [
      'tonic--split-handle',
      `tonic--split-${this.props.type}`
    ].join(' ')

    return this.html`
      ${this.elements[0]}

      <div class="${classes}" id="${this.handleId}">
      </div>

      ${this.elements[1]}
    `
  }
}

module.exports = {
  TonicSplit,
  TonicSplitLeft,
  TonicSplitRight,
  TonicSplitTop,
  TonicSplitBottom
}

},{"@optoolco/tonic":12}],23:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicSprite extends Tonic {
  static stylesheet () {
    return `
      tonic-sprite svg {
        visibility: hidden;
        height: 0;
      }
    `
  }

  render () {
    return this.html`
      <svg version="1.1" xmlns="http://www.w3.org/2000/svg">

        <symbol id="close" viewBox="0 0 100 100">
          <title>Close</title>
          <desc>Close Icon</desc>
          <path fill="currentColor" d="M70.7,22.6l-3.5-3.5c-0.1-0.1-0.3-0.1-0.4,0L50,45.9L23.2,19.1c-0.1-0.1-0.3-0.1-0.4,0l-3.5,3.5c-0.1,0.1-0.1,0.3,0,0.4
            l26.8,26.8L19.3,76.6c-0.1,0.1-0.1,0.3,0,0.4l3.5,3.5c0,0,0.1,0.1,0.2,0.1s0.1,0,0.2-0.1L50,53.6l25.9,25.9c0.1,0.1,0.3,0.1,0.4,0
            l3.5-3.5c0.1-0.1,0.1-0.3,0-0.4L53.9,49.8l26.8-26.8C80.8,22.8,80.8,22.7,80.7,22.6z"/>
        </symbol>

        <symbol id="danger" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M53.9,76.4h-7.6V68h7.6V76.4z M53.9,60.5h-7.6V25.6h7.6V60.5z"/>
        </symbol>

        <symbol id="warning" viewBox="0 0 100 100">
          <path fill="currentColor" d="M98.6,86.6l-46-79.7c-1.2-2-4-2-5.2,0l-46,79.7c-1.2,2,0.3,4.5,2.6,4.5h92C98.3,91.1,99.8,88.6,98.6,86.6z M53.9,80.4h-7.6V72h7.6V80.4z M53.9,64.5h-7.6V29.6h7.6V64.5z"/>
        </symbol>

        <symbol id="success" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M43.4,71.5L22,50.1l4.8-4.8L43.4,62l28.5-28.5l4.8,4.8L43.4,71.5z"/>
        </symbol>

        <symbol id="info" viewBox="0 0 100 100">
          <path fill="currentColor" d="M50.1,6.7C26.3,6.7,6.9,26.2,6.9,50s19.4,43.2,43.2,43.2c23.8,0,43.2-19.5,43.2-43.3C93.3,26.1,74,6.7,50.1,6.7z M54.1,75.5h-8.1v-7.8h8.1V75.5z M64.1,47.6c-0.8,1.1-2.4,2.7-4.8,4.5L57,54c-1.4,1.1-2.3,2.3-2.7,3.7c-0.3,0.8-0.4,2-0.4,3.6h-8c0.1-3.4,0.5-5.8,1-7.1c0.5-1.3,2-2.9,4.3-4.7l2.4-1.9c0.8-0.6,1.5-1.3,2-2.1c0.9-1.3,1.4-2.8,1.4-4.3c0-1.8-0.5-3.4-1.6-4.9c-1.1-1.5-3-2.3-5.8-2.3c-2.7,0-4.7,0.9-5.9,2.8c-1,1.6-1.6,3.3-1.7,5.1h-8.6c0.4-5.9,2.5-10.1,6.4-12.6l0,0c2.5-1.6,5.7-2.5,9.4-2.5c4.9,0,9,1.2,12.2,3.5c3.2,2.3,4.8,5.7,4.8,10.3C66.2,43.4,65.5,45.7,64.1,47.6z"/>
        </symbol>

        <symbol id="profileimage" viewBox="0 0 100 100">
          <rect fill="#F0F0F0" width="100" height="100"></rect>
          <circle fill="#D6D6D6" cx="49.3" cy="41.3" r="21.1"></circle>
          <path fill="#D6D6D6" d="M48.6,69.5c-18.1,0-33.1,13.2-36,30.5h72C81.8,82.7,66.7,69.5,48.6,69.5z"></path>
        </symbol>

        <symbol id="edit" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.8,32.8L67.5,20.5c-0.2-0.2-0.5-0.2-0.7,0L25.2,62.1c-0.1,0.1-0.1,0.1-0.1,0.2L20.8,79c0,0.2,0,0.4,0.1,0.5c0.1,0.1,0.2,0.1,0.4,0.1c0,0,0.1,0,0.1,0l16.6-4.4c0.1,0,0.2-0.1,0.2-0.1l41.6-41.6C79.9,33.3,79.9,33,79.8,32.8z M67.1,25.8l7.3,7.3L36.9,70.7l-7.3-7.3L67.1,25.8z M33,72.4l-6.8,1.8l1.8-6.9L33,72.4z"/>
        </symbol>

        <symbol id="checked" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.7,1H21.3C10.4,1,1.5,9.9,1.5,20.8v58.4C1.5,90.1,10.4,99,21.3,99h58.4c10.9,0,19.8-8.9,19.8-19.8V20.8C99.5,9.9,90.6,1,79.7,1z M93.3,79.3c0,7.5-6.1,13.6-13.6,13.6H21.3c-7.5,0-13.6-6.1-13.6-13.6V20.9c0-7.5,6.1-13.6,13.6-13.6V7.2h58.4c7.5,0,13.6,6.1,13.6,13.6V79.3z"/>
          <polygon points="44,61.7 23.4,41.1 17.5,47 44,73.5 85.1,32.4 79.2,26.5 "/>
        </symbol>

        <symbol id="download" viewBox="0 0 100 100">
          <path fill="currentColor" d="M49.5,64.2c0.1,0.1,0.2,0.1,0.4,0.1s0.3-0.1,0.4-0.1l12.4-12.4c0.1-0.1,0.1-0.2,0.1-0.4s-0.1-0.3-0.1-0.4l-2.1-2.1
            c-0.2-0.2-0.5-0.2-0.7,0l-7.9,7.9V27c0-0.3-0.2-0.5-0.5-0.5h-3c-0.3,0-0.5,0.2-0.5,0.5v29.8L40,48.9c-0.1-0.1-0.2-0.1-0.4-0.1
            s-0.3,0.1-0.4,0.1l-2.1,2.1c-0.2,0.2-0.2,0.5,0,0.7L49.5,64.2z"/>
          <path fill="currentColor" d="M68.9,69h-38c-0.3,0-0.5,0.2-0.5,0.5v3c0,0.3,0.2,0.5,0.5,0.5h38c0.3,0,0.5-0.2,0.5-0.5v-3C69.4,69.2,69.2,69,68.9,69z"/>
        </symbol>

        <symbol id="unchecked" viewBox="0 0 100 100">
          <path fill="currentColor" d="M79.7,99H21.3C10.4,99,1.5,90.1,1.5,79.2V20.8C1.5,9.9,10.4,1,21.3,1h58.4c10.9,0,19.8,8.9,19.8,19.8v58.4C99.5,90.1,90.6,99,79.7,99z M21.3,7.3c-7.5,0-13.6,6.1-13.6,13.6v58.4c0,7.5,6.1,13.6,13.6,13.6h58.4c7.5,0,13.6-6.1,13.6-13.6V20.8c0-7.5-6.1-13.6-13.6-13.6H21.3V7.3z"/>
        </symbol>

      </svg>
    `
  }
}

module.exports = { TonicSprite }

},{"@optoolco/tonic":12}],24:[function(require,module,exports){
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

},{"@optoolco/tonic":12}],25:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicTextarea extends Tonic {
  mutate (e) {
    const { width, height } = e.target.style

    this.state = {
      ...this.state,
      width,
      height
    }
  }

  defaults () {
    return {
      placeholder: '',
      spellcheck: true,
      disabled: false,
      required: false,
      readonly: false,
      autofocus: false,
      width: '100%',
      radius: '2px'
    }
  }

  static stylesheet () {
    return `
      tonic-textarea textarea {
        color: var(--tonic-primary, #333);
        width: 100%;
        font: 14px var(--tonic-monospace, 'Andale Mono', monospace);
        padding: 10px;
        background-color: var(--tonic-input-background, var(--tonic-background, transparent));
        border: 1px solid var(--tonic-border, #ccc);
        transition: border 0.2s ease-in-out;
        -webkit-appearance: none;
        -moz-appearance: none;
        appearance: none;
      }

      tonic-textarea textarea:invalid {
        border-color: var(--tonic-danger, #f66);
      }

      tonic-textarea textarea:focus {
        outline: none;
        background-color: var(--tonic-input-background-focus, rgba(241, 241, 241, 0.75));
      }

      tonic-textarea textarea[disabled] {
        background-color: transparent;
      }

      tonic-textarea label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader,  'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        padding-bottom: 10px;
        display: block;
      }

      tonic-textarea[edited] textarea[invalid],
      tonic-textarea[edited] textarea:invalid,
      tonic-textarea[edited] textarea[invalid]:focus,
      tonic-textarea[edited] textarea:invalid:focus {
        border-color: var(--tonic-error, #f66);
      }

      tonic-textarea[edited] textarea[invalid] ~ .tonic--invalid,
      tonic-textarea[edited] textarea:invalid ~ .tonic--invalid {
        transform: translateY(0);
        visibility: visible;
        opacity: 1;
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 1s ease 0s;
      }

      tonic-textarea textarea[disabled] {
        background-color: var(--tonic-background, #fff);
      }

      tonic-textarea[label] .tonic--invalid {
        margin-bottom: 13px;
      }

      tonic-textarea .tonic--invalid {
        font-size: 14px;
        text-align: center;
        margin-bottom: 13px;
        position: absolute;
        bottom: 100%;
        left: 0;
        right: 0;
        transform: translateY(-10px);
        transition: opacity 0.2s ease, transform 0.2s ease, visibility 0s ease 1s;
        visibility: hidden;
        opacity: 0;
      }

      tonic-textarea .tonic--invalid span {
        color: white;
        padding: 2px 6px;
        background-color: var(--tonic-error, #f66);
        border-radius: 2px;
        position: relative;
        display: inline-block;
        margin: 0 auto;
      }

      tonic-textarea .tonic--invalid span:after {
        content: '';
        width: 0;
        height: 0;
        display: block;
        position: absolute;
        bottom: -6px;
        left: 50%;
        transform: translateX(-50%);
        border-left: 6px solid transparent;
        border-right: 6px solid transparent;
        border-top: 6px solid var(--tonic-error, #f66);
      }

    `
  }

  styles () {
    const {
      width = this.state.width,
      height = this.state.height,
      radius,
      resize
    } = this.props

    return {
      textarea: {
        width,
        height,
        borderRadius: radius,
        resize: resize
      }
    }
  }

  get value () {
    return this.querySelector('textarea').value
  }

  set value (value) {
    this.querySelector('textarea').value = value
  }

  setValid () {
    const input = this.querySelector('textarea')
    if (!input) return

    input.setCustomValidity('')
    input.removeAttribute('invalid')
  }

  setInvalid (msg) {
    const input = this.querySelector('textarea')
    if (!input) return

    this.setAttribute('edited', true)
    this.state.edited = true

    input.setCustomValidity(msg)
    input.setAttribute('invalid', msg)
    const span = this.querySelector('.tonic--invalid span')
    if (!span) return

    span.textContent = msg
  }

  renderLabel () {
    if (typeof this.props.label === 'undefined') return ''
    return this.html`
      <label
        for="tonic--textarea_${this.props.id}"
      >${this.props.label}</label>
    `
  }

  willConnect () {
    const {
      value,
      persistSize
    } = this.props

    this.props.value = typeof value === 'undefined' ? this.textContent : value

    if (persistSize === 'true') {
      const cb = (changes) => this.mutate(changes.pop())
      this.observer = new window.MutationObserver(cb).observe(this, {
        attributes: true, subtree: true, attributeFilter: ['style']
      })
    }
  }

  disconnected () {
    if (this.observer) {
      this.observer.disconnect()
    }
  }

  keyup (e) {
    if (!this.props.pattern) {
      return
    }

    if (!this.regex) {
      this.regex = new RegExp(this.props.pattern)
    }

    const value = e.target.value.trim()

    value.match(this.regex)
      ? this.setValid()
      : this.setInvalid('Invalid')
  }

  render () {
    const {
      ariaLabelledby,
      autofocus,
      cols,
      height,
      disabled,
      label,
      maxlength,
      minlength,
      name,
      placeholder,
      readonly,
      required,
      rows,
      spellcheck,
      tabindex,
      theme,
      width
    } = this.props

    if (ariaLabelledby) this.removeAttribute('ariaLabelled')
    if (width) this.style.width = width
    if (height) this.style.height = height
    if (tabindex) this.removeAttribute('tabindex')
    if (theme) this.classList.add(`tonic--theme--${theme}`)
    if (name) this.removeAttribute('name')

    const errorMessage = this.props.errorMessage || 'Invalid'

    if (this.props.value === 'undefined') this.props.value = ''

    if (this.state.edited) {
      this.setAttribute('edited', true)
    }

    return this.html`
      <div class="tonic--wrapper">
        ${this.renderLabel()}
        <textarea ... ${{
          styles: 'textarea',
          id: `tonic--textarea_${this.props.id}`,
          'aria-label': label,
          'aria-labelledby': ariaLabelledby,
          cols,
          disabled: disabled === 'true',
          maxlength,
          minlength,
          name,
          placeholder,
          rows,
          spellcheck,
          tabindex,
          autofocus,
          required,
          readonly
        }}>${this.props.value}</textarea>
        <div class="tonic--invalid">
          <span id="tonic--error-${this.props.id}">${errorMessage}</span>
        </div>
      </div>
    `
  }
}

module.exports = { TonicTextarea }

},{"@optoolco/tonic":12}],26:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicToasterInline extends Tonic {
  defaults () {
    return {
      display: 'true'
    }
  }

  static stylesheet () {
    return `
      tonic-toaster-inline > div > .tonic--close svg,
      tonic-toaster-inline > div > .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-toaster-inline .tonic--notification {
        max-height: 0;
        background-color: var(--tonic-window, #fff);
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 3px;
        transform: translateY(-50%);
        transition: all 0.1s ease-in-out;
        opacity: 0;
        z-index: -1;
        position: absolute;
      }

      tonic-toaster-inline .tonic--notification.tonic--show {
        max-height: 100%;
        margin: 10px 0;
        transform: translateY(0);
        transition: all 0.1s ease-in-out;
        opacity: 1;
        z-index: 1;
        position: relative;
      }

      tonic-toaster-inline[animate="false"] .tonic--notification,
      tonic-toaster-inline[animate="false"] .tonic--notification.tonic--show {
        transition: none;
      }

      tonic-toaster-inline .tonic--notification.tonic--close {
        padding-right: 50px;
      }

      tonic-toaster-inline .tonic--notification.tonic--alert {
        padding-left: 35px;
      }

      tonic-toaster-inline .tonic--main {
        padding: 17px 18px 15px 18px;
      }

      tonic-toaster-inline.tonic--dismiss .tonic--main {
        margin-right: 40px;
      }

      tonic-toaster-inline .tonic--title {
        color: var(--tonic-primary, #333);
        font: 14px/18px var(--tonic-subheader, 'Arial', sans-serif);
      }

      tonic-toaster-inline .tonic--message {
        font: 14px/18px var(--tonic-subheader, 'Arial', sans-serif);
        color: var(--tonic-medium, #999);
      }

      tonic-toaster-inline .tonic--notification .tonic--icon {
        width: 16px;
        height: 16px;
        position: absolute;
        left: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
      }

      tonic-toaster-inline .tonic--close {
        width: 20px;
        height: 20px;
        position: absolute;
        right: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
        cursor: pointer;
      }
    `
  }

  show () {
    const node = this.querySelector('.tonic--notification')
    node.classList.add('tonic--show')
  }

  hide () {
    const node = this.querySelector('.tonic--notification')
    node.classList.remove('tonic--show')
  }

  click (e) {
    const el = Tonic.match(e.target, '.tonic--close')
    if (!el) return

    this.hide()
  }

  connected () {
    this.updated()
  }

  updated () {
    const {
      display,
      duration
    } = this.props

    if (display === 'true') {
      window.requestAnimationFrame(() => this.show())
    }

    if (duration) {
      setTimeout(() => this.hide(), duration)
    }
  }

  renderClose () {
    if (this.props.dismiss !== 'true') {
      return ''
    }

    this.classList.add('tonic--dismiss')

    return this.html`
      <div class="tonic--close">
        <svg>
          <use
            href="#close"
            xlink:href="#close"
            color="var(--tonic-primary, #333)"
            fill="var(--tonic-primary, #333)">
          </use>
        </svg>
      </div>
    `
  }

  renderIcon () {
    const type = this.props.type

    if (!type) return ''

    return this.html`
      <div class="tonic--icon">
        <svg>
          <use
            href="#${type}"
            xlink:href="#${type}"
            color="var(--tonic-${type}, #f66)"
            fill="var(--tonic-${type}, #f66)">
          </use>
        </svg>
      </div>
    `
  }

  styles () {
    return {
      type: {
        'border-color': `var(--tonic-${this.props.type}, #f66)`
      }
    }
  }

  render () {
    const {
      title,
      type,
      message,
      theme
    } = this.props

    if (theme) {
      this.setAttribute('theme', theme)
    }

    const classes = ['tonic--notification']

    if (type) {
      classes.push('tonic--alert', `tonic--${type}`)
    }

    return this.html`
      <div ... ${{
        class: classes.join(' '),
        styles: type ? 'type' : ''
      }}>
        ${this.renderIcon()}
        ${this.renderClose()}
        <div class="tonic--main">
          <div class="tonic--title">
            ${title}
          </div>
          <div class="tonic--message">
            ${message || this.childNodes}
          </div>
        </div>
      </div>
    `
  }
}

module.exports = { TonicToasterInline }

},{"@optoolco/tonic":12}],27:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicToaster extends Tonic {
  defaults () {
    return {
      position: 'center'
    }
  }

  static stylesheet () {
    return `
      tonic-toaster * {
        box-sizing: border-box;
      }

      tonic-toaster svg {
        width: 100%;
        height: 100%;
        position: absolute;
      }

      tonic-toaster .tonic--wrapper {
        user-select: none;
        position: fixed;
        top: 10px;
        display: flex;
        flex-wrap: wrap;
        flex-direction: column;
        visibility: hidden;
        z-index: 102;
      }

      @media (max-width: 850px) tonic-toaster .tonic--wrapper {
        width: 90%;
      }

      tonic-toaster .tonic--wrapper.tonic--show {
        visibility: visible;
      }

      tonic-toaster .tonic--wrapper.tonic--center {
        left: 50%;
        align-items: center;
        -webkit-transform: translateX(-50%);
        -ms-transform: translateX(-50%);
        transform: translateX(-50%);
      }

      tonic-toaster .tonic--wrapper.tonic--left {
        align-items: flex-start;
        left: 10px;
      }

      tonic-toaster .tonic--wrapper.tonic--right {
        align-items: flex-end;
        right: 10px;
      }

      tonic-toaster .tonic--notification {
        width: auto;
        max-width: 600px;
        margin-top: 10px;
        position: relative;
        background-color: var(--tonic-window);
        box-shadow: 0px 10px 40px -20px rgba(0,0,0,0.4), 0 0 1px #a2a9b1;
        border-radius: 3px;
        -webkit-transform: translateY(-100px);
        -ms-transform: translateY(-100px);
        transform: translateY(-100px);
        transition: opacity 0.2s ease, transform 0s ease 1s;
        z-index: 1;
        opacity: 0;
      }

      tonic-toaster .tonic--notification.tonic--show {
        opacity: 1;
        -webkit-transform: translateY(0);
        -ms-transform: translateY(0);
        transform: translateY(0);
        transition: transform 0.3s cubic-bezier(0.18, 0.89, 0.32, 1.28);
      }

      tonic-toaster .tonic--notification.tonic--close {
        padding-right: 50px;
      }

      tonic-toaster .tonic--notification.tonic--alert {
        padding-left: 35px;
      }

      tonic-toaster .tonic--main {
        padding: 17px 15px 15px 15px;
      }

      tonic-toaster .tonic--title {
        color: var(--tonic-primary);
        font: 14px/18px var(--tonic-subheader);
      }

      tonic-toaster .tonic--message {
        color: var(--tonic-medium);
        font: 14px/18px var(--tonic-body);
      }

      tonic-toaster .tonic--notification > .tonic--icon {
        width: 16px;
        height: 16px;
        position: absolute;
        left: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
      }

      tonic-toaster .tonic--notification > .tonic--close {
        width: 20px;
        height: 20px;
        position: absolute;
        right: 20px;
        top: 50%;
        -webkit-transform: translateY(-50%);
        -ms-transform: translateY(-50%);
        transform: translateY(-50%);
        cursor: pointer;
      }
    `
  }

  _getZIndex () {
    return Array.from(document.querySelectorAll('body *'))
      .map(elt => parseFloat(window.getComputedStyle(elt).zIndex))
      .reduce((z, highest = Number.MIN_SAFE_INTEGER) =>
        isNaN(z) || z < highest ? highest : z
      )
  }

  /**
   * @param {{
   *    message?: string,
   *    title: string,
   *    type: string,
   *    duration?: number,
   *    dismiss?: boolean
   * }} options
   */
  create (options) {
    options = options || {}
    const { message, title, duration, type } = options
    let dismiss = options.dismiss

    const notification = document.createElement('div')
    notification.className = 'tonic--notification'
    this.style.zIndex = this._getZIndex()

    const main = document.createElement('div')
    main.className = 'tonic--main'

    if (type) {
      notification.dataset.type = type
      notification.classList.add('tonic--alert')
    }

    const titleElement = document.createElement('div')
    titleElement.className = 'tonic--title'
    titleElement.textContent = title || this.props.title

    const messageElement = document.createElement('div')
    messageElement.className = 'tonic--message'
    messageElement.textContent = message || this.props.message

    if (typeof dismiss === 'string') {
      dismiss = dismiss === 'true'
    }

    if (dismiss !== false) {
      const closeIcon = document.createElement('div')
      closeIcon.className = 'tonic--close'
      notification.appendChild(closeIcon)
      notification.classList.add('tonic--close')

      const svgns = 'http://www.w3.org/2000/svg'
      const xlinkns = 'http://www.w3.org/1999/xlink'
      const svg = document.createElementNS(svgns, 'svg')
      const use = document.createElementNS(svgns, 'use')

      closeIcon.appendChild(svg)
      svg.appendChild(use)

      use.setAttributeNS(xlinkns, 'href', '#close')
      use.setAttributeNS(xlinkns, 'xlink:href', '#close')
      use.setAttribute('color', 'var(--tonic-primary)')
      use.setAttribute('fill', 'var(--tonic-primary)')
    }

    if (type) {
      const alertIcon = document.createElement('div')
      alertIcon.className = 'tonic--icon'
      notification.appendChild(alertIcon)

      const svgns = 'http://www.w3.org/2000/svg'
      const xlinkns = 'http://www.w3.org/1999/xlink'
      const svg = document.createElementNS(svgns, 'svg')
      const use = document.createElementNS(svgns, 'use')

      alertIcon.appendChild(svg)
      svg.appendChild(use)

      use.setAttributeNS(xlinkns, 'href', `#${type}`)
      use.setAttributeNS(xlinkns, 'xlink:href', `#${type}`)
      use.setAttribute('color', `var(--tonic-${type})`)
      use.setAttribute('fill', `var(--tonic-${type})`)
    }

    notification.appendChild(main)
    main.appendChild(titleElement)
    main.appendChild(messageElement)
    this.querySelector('.tonic--wrapper').appendChild(notification)
    this.show()

    setTimeout(() => {
      if (!notification) return
      notification.classList.add('tonic--show')
    }, 64)

    if (duration) {
      setTimeout(() => {
        if (!notification) return
        this.destroy(notification)
      }, duration)
    }
  }

  destroy (el) {
    el.classList.remove('tonic--show')
    el.addEventListener('transitionend', e => {
      if (el && el.parentNode) {
        el.parentNode.removeChild(el)
      }
    })
  }

  show () {
    const node = this.querySelector('.tonic--wrapper')
    node.classList.add('tonic--show')
  }

  hide () {
    const node = this.querySelector('.tonic--wrapper')
    node.classList.remove('tonic--show')
  }

  click (e) {
    const el = Tonic.match(e.target, '.tonic--close')
    if (!el) return

    const notification = el.closest('.tonic--notification')
    if (notification) this.destroy(notification)
  }

  render () {
    const {
      theme,
      position
    } = this.props

    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const classes = ['tonic--wrapper']
    if (position) classes.push(`tonic--${position}`)

    return this.html`
      <div class="${classes.join(' ')}">
      </div>
    `
  }
}

exports.TonicToaster = TonicToaster

},{"@optoolco/tonic":12}],28:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicToggle extends Tonic {
  constructor () {
    super()
    this._modified = false
  }

  get value () {
    const state = this.state
    const props = this.props

    const propsValue = typeof props.checked !== 'undefined' ? props.checked : props.value
    const stateValue = typeof state.checked !== 'undefined' ? state.checked : state.value
    let value

    if (this._modified) {
      value = typeof stateValue !== 'undefined' ? stateValue : propsValue
    } else {
      value = typeof propsValue !== 'undefined' ? propsValue : stateValue
    }

    return (value === true) || (value === 'true')
  }

  _setValue (value) {
    this._modified = true
    const checked = (value === true) || (value === 'true')

    this.state.checked = checked
  }

  set value (value) {
    this._setValue(value)
    this.reRender()
  }

  static stylesheet () {
    return `
      tonic-toggle .tonic--toggle--wrapper {
        height: 30px;
        position: relative;
      }

      tonic-toggle .tonic--toggle--wrapper > label {
        color: var(--tonic-medium, #999);
        font-weight: 500;
        font: 12px/14px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
        letter-spacing: 1px;
        margin-left: 58px;
        padding-top: 6px;
        display: block;
        user-select: none;
      }

      tonic-toggle .tonic--switch {
        position: absolute;
        left: 0;
        top: 0;
      }

      tonic-toggle .tonic--switch label:before {
        font: bold 12px var(--tonic-subheader, 'Arial', sans-serif);
        text-transform: uppercase;
      }

      tonic-toggle .tonic--toggle {
        position: absolute;
        opacity: 0;
        outline: none;
        user-select: none;
        z-index: 1;
      }

      tonic-toggle .tonic--toggle + label {
        width: 42px;
        height: 24px;
        padding: 2px;
        display: block;
        position: relative;
        background-color: var(--tonic-border, #ccc);
        border-radius: 60px;
        transition: background 0.2s ease-in-out;
        cursor: default;
      }

      tonic-toggle .tonic--toggle:focus + label {
        outline: -webkit-focus-ring-color auto 5px;
        outline-offset: 4px;
      }

      tonic-toggle .tonic--toggle + label:before {
        content: '';
        line-height: 29px;
        text-indent: 29px;
        position: absolute;
        top: 1px;
        left: 1px;
        right: 1px;
        bottom: 1px;
        display: block;
        border-radius: 60px;
        transition: background 0.2s ease-in-out;
        padding-top: 1px;
        font-size: 0.65em;
        letter-spacing: 0.05em;
        background-color: var(--tonic-border, #ccc);
      }

      tonic-toggle .tonic--toggle + label:after {
        content: '';
        width: 16px;
        position: absolute;
        top: 4px;
        left: 4px;
        bottom: 4px;
        background-color: var(--tonic-window, #fff);
        border-radius: 52px;
        transition: background 0.2s ease-in-out, margin 0.2s ease-in-out;
        display: block;
        z-index: 2;
      }

      tonic-toggle .tonic--toggle:disabled {
        cursor: default;
      }

      tonic-toggle .tonic--toggle:disabled + label {
        cursor: default;
        opacity: 0.5;
      }

      tonic-toggle .tonic--toggle:disabled + label:before {
        content: '';
        opacity: 0.5;
      }

      tonic-toggle .tonic--toggle:disabled + label:after {
        background-color: var(--tonic-window, #fff);
      }

      tonic-toggle .tonic--toggle:checked + label {
        background-color: var(--tonic-accent, #f66);
      }

      tonic-toggle .tonic--toggle:checked + label:before {
        background-color: var(--tonic-accent, #f66);
        color: var(--tonic-background);
      }

      tonic-toggle .tonic--toggle:checked + label:after {
        margin-left: 18px;
        background-color: var(--tonic-background, #fff);
      }
    `
  }

  change (e) {
    this._setValue(e.target.checked)
  }

  renderLabel () {
    const {
      id,
      label
    } = this.props

    if (!label) return ''

    return this.html`<label for="tonic--toggle--${id}">${label}</label>`
  }

  render () {
    const {
      id,
      disabled,
      theme,
      tabindex
    } = this.props

    if (tabindex) this.removeAttribute('tabindex')
    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const checked = this.value

    if (typeof this.state.checked === 'undefined') {
      this.state.checked = checked
    }

    return this.html`
      <div class="tonic--toggle--wrapper">
        <div class="tonic--switch">
          <input ... ${{
            type: 'checkbox',
            class: 'tonic--toggle',
            id: `tonic--toggle--${id}`,
            disabled: (disabled === true) || (disabled === 'true'),
            tabindex,
            checked
          }}/>
          <label for="tonic--toggle--${id}"></label>
        </div>
        ${this.renderLabel()}
      </div>
    `
  }
}

module.exports = { TonicToggle }

},{"@optoolco/tonic":12}],29:[function(require,module,exports){
const Tonic = require('@optoolco/tonic')

class TonicTooltip extends Tonic {
  connected () {
    const target = this.props.for
    const el = document.getElementById(target)

    const leave = e => {
      clearTimeout(TonicTooltip.timers[target])
      TonicTooltip.timers[target] = setTimeout(() => this.hide(), 256)
    }

    const enter = e => {
      clearTimeout(TonicTooltip.timers[target])
      this.show(el)
    }

    if (!el) {
      console.warn('Broken tooltip for: ' + target)
      return
    }

    el.addEventListener('mouseleave', leave)

    el.addEventListener('mousemove', enter)
    el.addEventListener('mouseenter', enter)

    this.addEventListener('mouseenter', e => {
      clearTimeout(TonicTooltip.timers[target])
    })

    this.addEventListener('mouseleave', leave)
  }

  defaults (props) {
    return {
      width: 'auto',
      height: 'auto'
    }
  }

  static stylesheet () {
    return `
      tonic-tooltip .tonic--tooltip {
        color: var(--tonic-primary, #333);
        position: fixed;
        background: var(--tonic-window, #fff);
        visibility: hidden;
        z-index: -1;
        opacity: 0;
        border: 1px solid var(--tonic-border, #ccc);
        border-radius: 2px;
        transition: visibility 0.2s ease, opacity 0.2s ease, z-index 0.2s ease, box-shadow 0.2s ease;
        willchange: visibility, opacity, box-shadow, z-index;
      }

      tonic-tooltip .tonic--tooltip.tonic--show {
        visibility: visible;
        opacity: 1;
        z-index: 1;
        box-shadow: 0px 30px 90px -20px rgba(0, 0, 0, 0.3);
      }

      tonic-tooltip .tonic--tooltip .tonic--tooltip-arrow {
        width: 12px;
        height: 12px;
        position: absolute;
        z-index: -1;
        background-color: var(--tonic-window, #fff);
        -webkit-transform: rotate(45deg);
        -ms-transform: rotate(45deg);
        transform: rotate(45deg);
        left: 50%;
      }

      tonic-tooltip .tonic--tooltip .tonic--tooltip-arrow {
        border: 1px solid transparent;
        border-radius: 2px;
        pointer-events: none;
      }

      tonic-tooltip .tonic--top .tonic--tooltip-arrow {
        margin-bottom: -6px;
        bottom: 100%;
        border-top-color: var(--tonic-border, #ccc);
        border-left-color: var(--tonic-border, #ccc);
      }

      tonic-tooltip .tonic--bottom .tonic--tooltip-arrow {
        margin-top: -6px;
        position: absolute;
        top: 100%;
        border-bottom-color: var(--tonic-border, #ccc);
        border-right-color: var(--tonic-border, #ccc);
      }
    `
  }

  show (triggerNode) {
    clearTimeout(TonicTooltip.timers[triggerNode.id])

    TonicTooltip.timers[triggerNode.id] = setTimeout(() => {
      const tooltip = this.querySelector('.tonic--tooltip')
      const arrow = this.querySelector('.tonic--tooltip-arrow')

      let { top, left } = triggerNode.getBoundingClientRect()

      left += triggerNode.offsetWidth / 2
      left -= tooltip.offsetWidth / 2

      const offset = triggerNode.offsetHeight + (arrow.offsetHeight / 2)

      if (top < (window.innerHeight / 2)) {
        tooltip.classList.remove('tonic--bottom')
        tooltip.classList.add('tonic--top')
        top += offset
      } else {
        tooltip.classList.remove('tonic--top')
        tooltip.classList.add('tonic--bottom')
        top -= offset + tooltip.offsetHeight
      }

      tooltip.style.top = `${top}px`
      tooltip.style.left = `${left}px`

      window.requestAnimationFrame(() => {
        tooltip.classList.add('tonic--show')
      })

      window.addEventListener('mousewheel', e => {
        this.hide()
      }, { once: true })
    }, 64)
  }

  hide (target) {
    this.firstElementChild.classList.remove('tonic--show')
  }

  styles () {
    const {
      width,
      height
    } = this.props

    return {
      tooltip: {
        width,
        height
      }
    }
  }

  render () {
    if (this.props.theme) {
      this.classList.add(`tonic--theme--${this.props.theme}`)
    }

    return this.html`
      <div class="tonic--tooltip" styles="tooltip">
        ${this.nodes}
        <span class="tonic--tooltip-arrow"></span>
      </div>
    `
  }
}

TonicTooltip.timers = {}

module.exports = { TonicTooltip }

},{"@optoolco/tonic":12}]},{},[9])(9)
});
