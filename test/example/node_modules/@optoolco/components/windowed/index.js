const Tonic = require('@optoolco/tonic')

class Windowed extends Tonic {
  constructor (o) {
    super(o)

    this.prependCounter = 0
    this.popCounter = 0
    this.shiftCounter = 0
    this.noMoreBottomRows = false
    this.currentVisibleRowIndex = -1
    this.prefetchDirection = null
    this.topIsTruncated = false
    this.bottomIsTruncated = false
    this.rowHeight = null
  }

  get length () {
    return this.rows.length
  }

  /**
   * defaults and their meaning :
   *  - prefetchThreshold, how many pages from bottom or top before
   *      triggering prefetch
   *  - rowsPerPage, how many rows per page to render
   *  - rowPadding, how many rows to render outside visible area
   *  - rowHeight, the height in pixels of each row for computations.
   *  - debug, if enabled will colorize pages.
   */
  defaults () {
    return {
      prefetchThreshold: 2,
      maxRowsLength: 10 * 1000,
      rowsPerPage: 100,
      rowPadding: 50,
      rowHeight: 30,
      debug: false
    }
  }

  styles () {
    return {
      inner: {
        position: 'relative'
      },
      top: {
        position: 'relative'
      },
      bottom: {
        position: 'relative'
      },

      outer: {
        width: '100%',
        height: 'inherit',
        overflow: 'auto'
      }
    }
  }

  getRows () {
    return this.rows
  }

  unshiftSOS () {
    this.noMoreTopRows = true
    const top = this.querySelector('.tonic--windowed--top')
    if (top) {
      top.innerHTML = ''
    }
  }

  pushEOS () {
    this.noMoreBottomRows = true
    const bottom = this.querySelector('.tonic--windowed--bottom')
    if (bottom) {
      bottom.innerHTML = ''
    }
  }

  push (o) {
    this.rows = this.rows || []
    this.rows.push(o)
  }

  unshift (o) {
    this.splice(0, 0, o)
  }

  pop () {
    this.rows = this.rows || []
    this.rows.pop()
  }

  shift () {
    this.rows = this.rows || []
    this.rows.shift()
  }

  find (fn) {
    if (!this.rows) return -1
    return this.rows.find(fn)
  }

  findIndex (fn) {
    if (!this.rows) return -1
    return this.rows.findIndex(fn)
  }

  splice () {
    if (!this.rows) return null

    const index = arguments[0]
    const totalItems = arguments.length - 2

    // If index === 0 then the user has not scrolled yet.
    // So do not auto scroll the table. If current visible row
    // is zero then the user just wants to look at the top
    // of the table.
    if (
      index <= this.currentVisibleRowIndex &&
      this.currentVisibleRowIndex !== 0
    ) {
      this.prependCounter += totalItems
      this.currentVisibleRowIndex += totalItems
    }

    return this.rows.splice.apply(this.rows, arguments)
  }

  getRow (idx) {
    return this.rows[idx]
  }

  async load (rows = []) {
    this.rows = rows
    this.noMoreBottomRows = false
    this.noMoreTopRows = false
    await this.reRender()

    const inner = this.querySelector('.tonic--windowed--inner')
    if (inner) {
      inner.innerHTML = ''
    }

    this.pages = {}
    this.rowHeight = parseInt(this.props.rowHeight, 10)
    this.pageHeight = this.props.rowsPerPage * this.rowHeight
    this.padding = this.props.rowPadding * this.rowHeight
    this.setInnerHeight()
    return this.rePaint()
  }

  checkMaxRows () {
    const maxRows = this.props.maxRowsLength
    if (maxRows % this.props.rowsPerPage !== 0) {
      throw new Error(
        'Invalid maxRowsLength value. Must be multiple of rowsPerPage'
      )
    }

    if (this.rows && this.rows.length > maxRows) {
      const toDelete = this.rows.length - maxRows

      if (this.prefetchDirection === 'bottom') {
        this.rows.splice(0, toDelete)
        this.noMoreTopRows = false
        this.shiftCounter += toDelete

        this.topIsTruncated = true
        this.bottomIsTruncated = false
        if (this.onTopTruncate) {
          this.onTopTruncate()
        }
      } else if (
        this.prefetchDirection === 'top'
      // || this.prefetchDirection === null
      ) {
        this.rows.length = maxRows
        this.noMoreBottomRows = false
        this.popCounter += toDelete

        this.bottomIsTruncated = true
        this.topIsTruncated = false
        if (this.onBottomTruncate) {
          this.onBottomTruncate()
        }
      }
    }
  }

  setInnerHeight () {
    this.pages = this.pages || {}
    this.pagesAvailable = this.pagesAvailable || []
    const outer = this.querySelector('.tonic--windowed--outer')
    if (!outer) return

    this.checkMaxRows()
    this.outerHeight = outer.offsetHeight
    this.numPages = Math.ceil(this.rows.length / this.props.rowsPerPage)

    const inner = this.querySelector('.tonic--windowed--inner')
    inner.style.height = `${this.rowHeight * this.rows.length}px`
  }

  setHeight (height, { render } = {}) {
    const outer = this.querySelector('.tonic--windowed--outer')
    if (!outer) return

    outer.style.height = height
    this.outerHeight = outer.offsetHeight

    if (render !== false) {
      this.rePaint()
    }
  }

  getPage (i) {
    let page, state

    if (this.pages[i]) {
      page = this.pages[i]
      state = 'ok'
    } else if (this.pagesAvailable.length) {
      page = this.getAvailablePage()
      state = 'old'
    } else {
      page = this.createNewPage()
      state = 'fresh'
    }

    this.pages[i] = page

    page.style.height = i < this.numPages - 1
      ? `${this.pageHeight}px`
      : this.getLastPageHeight()

    page.style.top = this.getPageTop(i)
    return [page, state]
  }

  getAvailablePage () {
    const page = this.pagesAvailable.pop()
    const inner = this.querySelector('.tonic--windowed--inner')
    inner.appendChild(page)
    return page
  }

  /**
   * Logic for scrolling to an offset. This is nuanced because
   * we try to avoid touching `offsetTop` to avoid causing unnecessary
   * repaints or layout calculations.
   *
   * @param {number} offsetId
   */
  scrollToId (offsetId) {
    const index = this.rows.findIndex(o => o.id === offsetId)
    if (typeof index !== 'number') return
    if (this.rowHeight === null) {
      throw new Error('Cannot call scrollToId() before load()')
    }

    const scrollTop = index * this.rowHeight

    const outer = this.querySelector('.tonic--windowed--outer')
    if (!outer) {
      throw new Error('Cannot call scrollToId() in empty or loading state')
    }

    outer.scrollTop = scrollTop
    this.rePaint({ fromScroll: true, scrollTop: scrollTop })
  }

  createNewPage () {
    const page = document.createElement('div')

    Object.assign(page.style, {
      position: 'absolute',
      minWidth: '100%',
      className: 'tonic--windowed--page'
    })

    if (this.props.debug) {
      const random = Math.random() * 356
      page.style.backgroundColor = `hsla(${random}, 100%, 50%, 0.5)`
    }

    const inner = this.querySelector('.tonic--windowed--inner')
    inner.appendChild(page)
    page.__overflow__ = []
    return page
  }

  rePaint ({ refresh, load, fromScroll, scrollTop } = {}) {
    if (refresh && load !== false) this.load(this.rows)

    const outer = this.querySelector('.tonic--windowed--outer')
    if (!outer) return

    this.checkMaxRows()
    const viewStart = typeof scrollTop === 'number' ? scrollTop : outer.scrollTop
    const viewEnd = viewStart + this.outerHeight

    const _start = Math.floor((viewStart - this.padding) / this.pageHeight)
    const start = Math.max(_start, 0) || 0

    const _end = Math.floor((viewEnd + this.padding) / this.pageHeight)
    const end = Math.min(_end, this.numPages - 1)
    const pagesRendered = {}

    for (let i = start; i <= end; i++) {
      const [page, state] = this.getPage(i)

      if (state === 'fresh') {
        this.fillPage(i)
      } else if (refresh || state === 'old') {
        if (this.updateRow) {
          this.updatePage(i)
        } else {
          page.innerHTML = ''
          page.__overflow__ = []
          this.fillPage(i)
        }
      }
      pagesRendered[i] = true
    }

    const inner = this.querySelector('.tonic--windowed--inner')

    for (const pageKey of Object.keys(this.pages)) {
      if (pagesRendered[pageKey]) continue

      this.pagesAvailable.push(this.pages[pageKey])
      inner.removeChild(this.pages[pageKey])
      // TODO: Figure this out at boundaries.
      //    When we are scrolling and triggering `prefetchBottom()`
      //    we are deleting this.pages too much causing unnecessary
      //    work to happen.
      delete this.pages[pageKey]
    }

    let currentScrollTop = viewStart
    if (this.state.scrollTop && !fromScroll) {
      currentScrollTop = this.state.scrollTop
      outer.scrollTop = this.state.scrollTop
    }

    let shiftHappened = false
    let popHappened = false
    if (
      this.prependCounter > 0 ||
      this.shiftCounter > 0 ||
      this.popCounter > 0
    ) {
      // TODO: This logic is fragile / has edge cases.
      currentScrollTop += this.prependCounter * this.rowHeight
      currentScrollTop -= this.shiftCounter * this.rowHeight
      outer.scrollTop = currentScrollTop
      this.state.scrollTop = currentScrollTop

      if (this.shiftCounter > 0) {
        shiftHappened = true
      }
      if (this.popCounter > 0) {
        popHappened = true
      }

      /**
       * TODO: @Raynos when we mutate `outer.scrollTop` the
       *    windowed component cannot correctly recycle and
       *    re-use `this.pages` ; This leads to unnecessary
       *    updatePage() or even creation of new pages.
       */
      this.prependCounter = 0
      this.shiftCounter = 0
      this.popCounter = 0
    }

    // Set the current visible row index used for tracking
    // prepends.
    this.currentVisibleRowIndex = Math.floor(
      currentScrollTop / this.rowHeight
    )

    const totalHeight = this.rows.length * this.props.rowHeight
    if (
      viewEnd === totalHeight &&
      this.prefetchBottom &&
      this.renderLoadingBottom &&
      !this.noMoreBottomRows
    ) {
      const bottom = this.querySelector('.tonic--windowed--bottom')
      bottom.innerHTML = this.renderLoadingBottom()
    }

    if (
      this.rows.length === this.props.maxRowsLength &&
      viewStart === 0 &&
      this.prefetchTop &&
      this.renderLoadingTop &&
      !this.noMoreTopRows
    ) {
      const top = this.querySelector('.tonic--windowed--top')
      top.innerHTML = this.renderLoadingTop()
    }

    // TODO: Sometimes prefetchTop() does not get called.
    if (!popHappened && (
      this.rows.length === this.props.maxRowsLength &&
      start <= this.props.prefetchThreshold
    )) {
      if (!this.noMoreTopRows && this.prefetchTop) {
        this.prefetchDirection = 'top'
        this.prefetchTop()
      }
    }

    if (!shiftHappened && (
      end >= this.numPages - this.props.prefetchThreshold
    )) {
      if (!this.noMoreBottomRows && this.prefetchBottom) {
        this.prefetchDirection = 'bottom'
        this.prefetchBottom()
      }
    }
  }

  getPageTop (i) {
    return `${i * this.pageHeight}px`
  }

  getLastPageHeight () {
    return `${(this.rows.length % this.props.rowsPerPage) * this.rowHeight}px`
  }

  fillPage (i) {
    const page = this.pages[i]
    const frag = document.createDocumentFragment()
    const limit = Math.min((i + 1) * this.props.rowsPerPage, this.rows.length)

    for (let j = i * this.props.rowsPerPage; j < limit; j++) {
      const data = this.getRow(j)
      if (!data) continue

      const div = document.createElement('div')
      div.innerHTML = this.renderRow(data, j)
      frag.appendChild(div.firstElementChild)
    }

    page.appendChild(frag)
  }

  updatePage (i) {
    const page = this.pages[i]
    const start = i * parseInt(this.props.rowsPerPage, 10)
    const limit = Math.min((i + 1) * this.props.rowsPerPage, this.rows.length)

    const inner = this.querySelector('.tonic--windowed--inner')

    if (start > limit) {
      inner.removeChild(page)
      delete this.pages[i]
      return
    }

    for (let j = start, rowIdx = 0; j < limit; j++, rowIdx++) {
      if (page.children[rowIdx] && this.updateRow) {
        this.updateRow(this.getRow(j), j, page.children[rowIdx])
      } else if (page.__overflow__.length > 0 && this.updateRow) {
        const child = page.__overflow__.shift()
        this.updateRow(this.getRow(j), j, child)
        page.appendChild(child)
      } else {
        const div = document.createElement('div')
        div.innerHTML = this.renderRow(this.getRow(j), j)
        page.appendChild(div.firstElementChild)
      }
    }

    while (page.children.length > limit - start) {
      const child = page.lastChild
      page.__overflow__.push(child)
      page.removeChild(child)
    }
  }

  connected () {
    if (!this.props.data || !this.props.data.length) return
    this.load(this.props.data)
  }

  updated () {
    const outer = this.querySelector('.tonic--windowed--outer')

    if (outer && outer.__hasWindowedScrollListener) return
    if (outer) {
      outer.addEventListener('scroll', () => {
        const scrollTop = this.state.scrollTop = outer.scrollTop
        this.rePaint({ fromScroll: true, scrollTop: scrollTop })
      }, { passive: true })
      outer.__hasWindowedScrollListener = true
    }
  }

  renderLoadingState () {
    return this.html`<div class="tonic--windowed--loader"></div>`
  }

  renderEmptyState () {
    return this.html`<div class="tonic--windowed--empty"></div>`
  }

  render () {
    if (!this.rows) {
      return this.renderLoadingState()
    }

    if (!this.rows.length) {
      return this.renderEmptyState()
    }

    return this.html`
      <div class="tonic--windowed--outer" styles="outer">
        <div class="tonic--windowed--top" styles="top">
        </div>
        <div class="tonic--windowed--inner" styles="inner">
        </div>
        <div class="tonic--windowed--bottom" styles="bottom">
        </div>
      </div>
    `
  }
}

module.exports = { Windowed }
