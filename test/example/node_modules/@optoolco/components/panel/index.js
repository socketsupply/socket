const { TonicDialog } = require('../dialog')

class TonicPanel extends TonicDialog {
  constructor () {
    super()

    this.classList.add('tonic--panel')
  }

  defaults () {
    return {
      position: 'right'
    }
  }

  stylesheet () {
    const {
      width,
      position
    } = this.props

    const range = [0, width]

    if (position !== 'right') {
      range[1] = `-${width}`
    }

    return `
      .tonic--dialog.tonic--panel,
      .tonic--dialog.tonic--show.tonic--panel {
        left: unset;
        border-radius: 0;
        top: 0;
        width: ${width};
        bottom: 0;
        ${position}: 0;
        transform: translateX(${range[0]});
        animation-name: tonic--panel--show;
      }

      .tonic--dialog.tonic--hide.tonic--panel {
        opacity: 0;
        transform: translateX(${width});
        animation-name: tonic--panel--hide;
      }

      @keyframes tonic--panel--show {
        from {
          opacity: 0;
          transform: translateX(${range[1]});
        }

        to {
          opacity: 1;
          transform: translateX(${range[0]});
        }
      }

      @keyframes tonic--panel--hide {
        from {
          opacity: 1;
          transform: translateX(${range[0]});
        }

        to {
          opacity: 0;
          transform: translateX(${range[1]});
        }
      }
    `
  }
}

module.exports = { TonicPanel }
