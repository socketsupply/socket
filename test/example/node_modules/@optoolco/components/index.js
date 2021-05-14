let Tonic
try {
  Tonic = require('@optoolco/tonic')
} catch (err) {
  console.error('Missing dependency. Try `npm install @optoolco/tonic`.')
  throw err
}

const version = Tonic.version
const major = version ? version.split('.')[0] : '0'

if (parseInt(major, 10) < 12) {
  console.error('Out of date dependency. Try `npm install @optoolco/tonic@12`.')
  throw new Error('Invalid Tonic version. requires at least v12')
}

const { TonicAccordion, TonicAccordionSection } = require('./accordion')
const { TonicBadge } = require('./badge')
const { TonicButton } = require('./button')
const { TonicChart } = require('./chart')
const { TonicCheckbox } = require('./checkbox')
const { TonicDialog } = require('./dialog')
const { TonicForm } = require('./form')
const { TonicIcon } = require('./icon')
const { TonicInput } = require('./input')
const { TonicLoader } = require('./loader')
const { TonicPanel } = require('./panel')
const { TonicPopover } = require('./popover')
const { TonicProfileImage } = require('./profile-image')
const { TonicProgressBar } = require('./progress-bar')
const { TonicRange } = require('./range')
const { TonicRelativeTime } = require('./relative-time')
const { TonicRouter } = require('./router')
const { TonicSelect } = require('./select')
const { TonicSprite } = require('./sprite')
const { TonicSplit, TonicSplitLeft, TonicSplitRight, TonicSplitTop, TonicSplitBottom } = require('./split')
const { TonicTabs, TonicTab, TonicTabPanel } = require('./tabs')
const { TonicTextarea } = require('./textarea')
const { TonicTooltip } = require('./tooltip')
const { TonicToasterInline } = require('./toaster-inline')
const { TonicToaster } = require('./toaster')
const { TonicToggle } = require('./toggle')

let once = false

//
// An example collection of components.
//
module.exports = components
// For supporting unpkg / dist / jsfiddle.
components.Tonic = Tonic

function components (Tonic) {
  if (once) {
    return
  }
  once = true

  Tonic.add(TonicAccordion)
  Tonic.add(TonicAccordionSection)
  Tonic.add(TonicBadge)
  Tonic.add(TonicButton)
  Tonic.add(TonicChart)
  Tonic.add(TonicCheckbox)
  Tonic.add(TonicDialog)
  Tonic.add(TonicForm)
  Tonic.add(TonicInput)
  Tonic.add(TonicIcon)
  Tonic.add(TonicLoader)
  Tonic.add(TonicPanel)
  Tonic.add(TonicPopover)
  Tonic.add(TonicProfileImage)
  Tonic.add(TonicProgressBar)
  Tonic.add(TonicRange)
  Tonic.add(TonicRelativeTime)
  Tonic.add(TonicRouter)
  Tonic.add(TonicSelect)
  Tonic.add(TonicSprite)
  Tonic.add(TonicSplit)
  Tonic.add(TonicSplitLeft)
  Tonic.add(TonicSplitRight)
  Tonic.add(TonicSplitTop)
  Tonic.add(TonicSplitBottom)
  Tonic.add(TonicTabPanel)
  Tonic.add(TonicTab)
  Tonic.add(TonicTabs)
  Tonic.add(TonicTextarea)
  Tonic.add(TonicTooltip)
  Tonic.add(TonicToasterInline)
  Tonic.add(TonicToaster)
  Tonic.add(TonicToggle)
}
