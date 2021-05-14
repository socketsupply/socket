const Tonic = require('@optoolco/tonic')

class TonicChart extends Tonic {
  static stylesheet () {
    return `
      tonic-chart {
        display: inline-block;
        position: relative;
      }

      tonic-chart canvas {
        display: inline-block;
        position: relative;
      }
    `
  }

  set library (Chart) {
    this.Chart = Chart
  }

  draw (data = {}, options = {}) {
    const root = this.querySelector('canvas')
    const type = this.props.type || options.type

    if (!this.Chart) {
      console.error('No chart constructor found')
      return
    }

    return new this.Chart(root, {
      type,
      options,
      data
    })
  }

  async redraw () {
    return this.connected()
  }

  async fetch (url, opts = {}) {
    if (!url) return {}

    try {
      const res = await window.fetch(url, opts)
      return { data: await res.json() }
    } catch (err) {
      return { err }
    }
  }

  async connected () {
    let data = null
    if (this.props.library) {
      this.Chart = this.props.library
    }

    if (!this.Chart) return

    const options = {
      ...this.props,
      ...this.props.options
    }

    const src = this.props.src

    if (typeof src === 'string') {
      const response = await this.fetch(src)

      if (response.err) {
        console.error(response.err)
        data = {}
      } else {
        data = response.data
      }
    }

    if (src === Object(src)) {
      data = src
    }

    if (data && data.chartData) {
      throw new Error('chartData propery deprecated')
    }

    if (data) {
      this.draw(data, options)
    }
  }

  render () {
    const {
      width,
      height
    } = this.props

    this.style.width = width
    this.style.height = height

    return this.html`
      <canvas ... ${{ width, height }}>
      </canvas>
    `
  }
}

module.exports = { TonicChart }
