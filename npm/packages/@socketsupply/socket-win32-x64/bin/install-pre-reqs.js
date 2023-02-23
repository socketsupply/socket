#!/usr/bin/env node

import { execSync as exec, spawn } from 'node:child_process'

async function main() {
  let pscmd = '"cd ps1 && powershell .\\install.ps1 -shbuild:$false && exit"';
  let npm_cmd = `%comspec% /k ${pscmd}`;
  process.stdout.write(npm_cmd);

  const c = spawn(
    '%comspec%',
    ['/k', pscmd],
    { detached: true, shell: true, windowsHide: false }
  );

  await new Promise((resolve) => {
    c.on('close', resolve);
    process.stdout.write("npm completed");
  });
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
