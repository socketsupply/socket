const Tonic = require('@optoolco/tonic')

class TonicProfileImage extends Tonic {
  get value () {
    const state = this.state
    return state.data || this.props.src
  }

  defaults () {
    return {
      src: TonicProfileImage.svg.default(),
      size: '50px',
      radius: '5px'
    }
  }

  static stylesheet () {
    return `
      tonic-profile-image {
        display: inline-block;
      }

      tonic-profile-image .tonic--wrapper {
        position: relative;
        overflow: hidden;
      }

      tonic-profile-image .tonic--image {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-size: cover;
        background-position: center center;
        background-repeat: no-repeat;
      }

      tonic-profile-image .tonic--overlay {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0,0,0,0.5);
        transition: opacity 0.2s ease-in-out;
        visibility: hidden;
        opacity: 0;
        display: flex;
      }

      tonic-profile-image .tonic--overlay .tonic--icon {
        width: 30px;
        height: 30px;
        position: absolute;
        top: 50%;
        left: 50%;
        -webkit-transform: translateX(-50%) translateY(-50%);
        -moz-transform: translateX(-50%) translateY(-50%);
        transform: translateX(-50%) translateY(-50%);
      }

      tonic-profile-image .tonic--overlay .tonic--icon svg {
        width: inherit;
        height: inherit;
      }

      tonic-profile-image .tonic--wrapper.tonic--editable:hover .tonic--overlay {
        visibility: visible;
        opacity: 1;
        cursor: pointer;
      }
    `
  }

  styles () {
    const {
      src,
      size,
      border,
      radius
    } = this.props

    return {
      background: {
        backgroundImage: `url('${src}')`
      },
      hidden: {
        display: 'none'
      },
      wrapper: {
        width: size,
        height: size,
        border: border,
        borderRadius: radius
      }
    }
  }

  getPictureData (src, cb) {
    const reader = new window.FileReader()
    reader.onerror = err => cb(err)
    reader.onload = e => cb(null, e.target.result)
    reader.readAsDataURL(src)
  }

  click (e) {
    if (this.props.editable) {
      if (this.props.editable === 'false') return
      const fileInput = this.getElementsByTagName('input')[0]
      fileInput.click()
    }
  }

  change (e) {
    const fileInput = this.getElementsByTagName('input')[0]
    const data = fileInput.files[0]
    if (e.data) return
    e.stopPropagation()

    const {
      size,
      type,
      path,
      lastModifiedDate
    } = data

    this.getPictureData(data, (err, data) => {
      if (err) {
        const event = new window.Event('error')
        event.message = err.message
        this.dispatchEvent(event)
        return
      }

      const slot = this.querySelector('.tonic--image')

      this.state.size = size
      this.state.path = path
      this.state.time = type
      this.state.mtime = lastModifiedDate
      this.state.data = data

      slot.style.backgroundImage = 'url("' + data + '")'
      const event = new window.Event('change', { bubbles: true })
      event.data = true // prevent recursion
      this.dispatchEvent(event)
    })
  }

  render () {
    const {
      theme,
      editable
    } = this.props

    if (theme) this.classList.add(`tonic--theme--${theme}`)

    const classes = ['tonic--wrapper']
    if (editable === 'true') classes.push('tonic--editable')

    return this.html`
      <div
        class="${classes.join(' ')}"
        styles="wrapper">

        <div
          class="tonic--image"
          styles="background">
        </div>

        <input type="file" styles="hidden"/>

        <div class="tonic--overlay">
          <div class="tonic--icon">
            <svg>
              <use
                href="#edit"
                xlink:href="#edit"
                color="#fff"
                fill="#fff">
              </use>
            </svg>
          </div>
        </div>
      </div>
    `
  }
}

TonicProfileImage.svg = {}
TonicProfileImage.svg.toURL = s => `data:image/svg+xml;base64,${window.btoa(s)}`
TonicProfileImage.svg.default = () => TonicProfileImage.svg.toURL(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
    <rect fill="#F0F0F0" width="100" height="100"></rect>
    <circle fill="#D6D6D6" cx="49.3" cy="41.3" r="21.1"></circle>
    <path fill="#D6D6D6" d="M48.6,69.5c-18.1,0-33.1,13.2-36,30.5h72C81.8,82.7,66.7,69.5,48.6,69.5z"></path>
  </svg>
`)

module.exports = { TonicProfileImage }
