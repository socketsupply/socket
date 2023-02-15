import { test } from 'socket:test'
import ApplicationWindow, { constants, formatFileUrl } from 'socket:window'
import process from 'socket:process'

test('window constants', (t) => {
  t.equal(ApplicationWindow.constants.WINDOW_ERROR, -1, 'ApplicationWindow.constants.WINDOW_ERROR is -1')
  t.equal(ApplicationWindow.constants.WINDOW_NONE, 0, 'ApplicationWindow.constants.WINDOW_NONE is 0')
  t.equal(ApplicationWindow.constants.WINDOW_CREATING, 10, 'ApplicationWindow.constants.WINDOW_CREATING is 10')
  t.equal(ApplicationWindow.constants.WINDOW_CREATED, 11, 'ApplicationWindow.constants.WINDOW_CREATED is 11')
  t.equal(ApplicationWindow.constants.WINDOW_HIDING, 20, 'ApplicationWindow.constants.WINDOW_HIDING is 20')
  t.equal(ApplicationWindow.constants.WINDOW_HIDDEN, 21, 'ApplicationWindow.constants.WINDOW_HIDDEN is 21')
  t.equal(ApplicationWindow.constants.WINDOW_SHOWING, 30, 'ApplicationWindow.constants.WINDOW_SHOWING is 30')
  t.equal(ApplicationWindow.constants.WINDOW_SHOWN, 31, 'ApplicationWindow.constants.WINDOW_SHOWN is 31')
  t.equal(ApplicationWindow.constants.WINDOW_CLOSING, 40, 'ApplicationWindow.constants.WINDOW_CLOSING is 40')
  t.equal(ApplicationWindow.constants.WINDOW_CLOSED, 41, 'ApplicationWindow.constants.WINDOW_CLOSED is 41')
  t.equal(ApplicationWindow.constants.WINDOW_EXITING, 50, 'ApplicationWindow.constants.WINDOW_EXITING is 50')
  t.equal(ApplicationWindow.constants.WINDOW_EXITED, 51, 'ApplicationWindow.constants.WINDOW_EXITED is 51')
  t.equal(ApplicationWindow.constants.WINDOW_KILLING, 60, 'ApplicationWindow.constants.WINDOW_KILLING is 60')
  t.equal(ApplicationWindow.constants.WINDOW_KILLED, 61, 'ApplicationWindow.constants.WINDOW_KILLED is 61')

  t.equal(constants.WINDOW_ERROR, ApplicationWindow.constants.WINDOW_ERROR)
  t.equal(constants.WINDOW_NONE, ApplicationWindow.constants.WINDOW_NONE)
  t.equal(constants.WINDOW_CREATING, ApplicationWindow.constants.WINDOW_CREATING)
  t.equal(constants.WINDOW_CREATED, ApplicationWindow.constants.WINDOW_CREATED)
  t.equal(constants.WINDOW_HIDING, ApplicationWindow.constants.WINDOW_HIDING)
  t.equal(constants.WINDOW_HIDDEN, ApplicationWindow.constants.WINDOW_HIDDEN)
  t.equal(constants.WINDOW_SHOWING, ApplicationWindow.constants.WINDOW_SHOWING)
  t.equal(constants.WINDOW_SHOWN, ApplicationWindow.constants.WINDOW_SHOWN)
  t.equal(constants.WINDOW_CLOSING, ApplicationWindow.constants.WINDOW_CLOSING)
  t.equal(constants.WINDOW_CLOSED, ApplicationWindow.constants.WINDOW_CLOSED)
  t.equal(constants.WINDOW_EXITING, ApplicationWindow.constants.WINDOW_EXITING)
  t.equal(constants.WINDOW_EXITED, ApplicationWindow.constants.WINDOW_EXITED)
  t.equal(constants.WINDOW_KILLING, ApplicationWindow.constants.WINDOW_KILLING)
  t.equal(constants.WINDOW_KILLED, ApplicationWindow.constants.WINDOW_KILLED)
})

test('formatFileUrl', (t) => {
  t.equal(formatFileUrl('index.html'), `file://${process.cwd()}/index.html`)
})
