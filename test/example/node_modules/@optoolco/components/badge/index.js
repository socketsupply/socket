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
