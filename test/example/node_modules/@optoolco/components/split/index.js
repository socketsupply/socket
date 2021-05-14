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
