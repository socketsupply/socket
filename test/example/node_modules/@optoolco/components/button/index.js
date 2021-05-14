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
