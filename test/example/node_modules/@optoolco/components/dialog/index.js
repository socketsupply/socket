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
