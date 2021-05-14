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
