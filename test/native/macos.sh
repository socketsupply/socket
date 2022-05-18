#!/usr/bin/osascript -l JavaScript
'use strict'

function run (args) {
	var script = Application.currentApplication()
	script.includeStandardAdditions = true
	script.displayNotification('JXA library!', { withTitle: 'test', subtitle: 'Done' })
	script.displayAlert('library tested:' + JSON.stringify(args))
	return args
}
