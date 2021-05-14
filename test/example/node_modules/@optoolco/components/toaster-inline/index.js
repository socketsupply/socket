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
