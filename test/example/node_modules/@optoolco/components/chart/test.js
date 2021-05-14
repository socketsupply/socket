const tape = require('@pre-bundled/tape')
const { qs } = require('qsa-min')

const { html } = require('../_test/util')
const components = require('..')
components(require('@optoolco/tonic'))

// const CHART_OPTS = {
//   tooltips: {
//     enabled: false
//   },
//   legend: {
//     display: false
//   },
//   drawTicks: true,
//   drawBorder: true
// }

const CHART_DATA = {
  labels: ['Foo', 'Bar', 'Bazz'],
  datasets: [{
    label: 'Quxx (millions)',
    backgroundColor: ['#c3c3c3', '#f06653', '#8f8f8f'],
    data: [278, 467, 34]
  }]
}

document.body.appendChild(html`
<section id="chart">
  <h2>Chart</h2>

  <div id="chart-1" class="test-container">
    <span>Default</span>
    <tonic-chart
      type="horizontalBar"
      width=400px
      height=400px
      src="${CHART_DATA}"
      library="${require('chart.js')}"
    ></tonic-chart>
  </div>
</section>
`)

tape('got a chart', t => {
  const container = qs('#chart-1')
  const chart = qs('#chart-1 tonic-chart')
  const canvas = qs('#chart-1 canvas')

  t.ok(container)
  t.ok(chart)
  t.ok(canvas)

  t.equal(canvas.width, 400)
  t.equal(canvas.height, 400)

  const styles = window.getComputedStyle(canvas)
  t.equal(styles.display, 'block')
  t.equal(canvas.getAttribute('class'), 'chartjs-render-monitor')

  t.end()
})
