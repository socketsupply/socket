#!/usr/bin/env node

import assert from 'assert'

const [platform, arch] = process.argv.slice(2)
assert(process.platform === platform && process.arch === arch)
