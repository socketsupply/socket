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
