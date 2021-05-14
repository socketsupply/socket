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
