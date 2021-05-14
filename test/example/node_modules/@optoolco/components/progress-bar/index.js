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
