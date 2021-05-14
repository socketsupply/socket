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
