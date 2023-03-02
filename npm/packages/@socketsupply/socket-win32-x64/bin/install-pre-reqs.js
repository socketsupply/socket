#!/usr/bin/env node

import { spawn } from 'node:child_process'
import fs from 'node:fs'
import path from 'node:path'

async function insert_env_vars() {
  if (!fs.existsSync('ps1/env.json')) {
    console.log('WARNING: No env.json, builds will fail - please report (https://discord.gg/YPV32gKCsH)');
    return;
  }

  let env_json = fs.readFileSync('ps1/env.json', "utf16le");
  while (!env_json.startsWith(' ') && !env_json.startsWith('{')) {
    env_json = env_json.substring(1);
  }

  var env = JSON.parse(env_json);

  var envs = '';
  for (const e in env) {
    envs += `SET ${e}=${env[e]}\r\n`;
  }

  console.log(envs);

  fs.writeFileSync("env.bat", envs);

  var ssc_cmd_path = path.join(process.cwd(), '../../.bin/ssc.cmd');


  if (!fs.existsSync(ssc_cmd_path)) {
    console.log(`WARNING: No ${ssc_cmd_path} builds will fail - please report (https://discord.gg/YPV32gKCsH)`);
    return;
  }

  fs.writeFileSync(ssc_cmd_path, envs + fs.readFileSync(ssc_cmd_path), "utf-8");
}

async function main() {
  let pscmd = '"cd ps1 && powershell .\\install.ps1 -shbuild:$false -package_setup:$true "';
  let npm_cmd = `%comspec% /k ${pscmd}`;
  process.stdout.write(npm_cmd + '\n');

  const c = spawn(
    '%comspec%',
    ['/k', pscmd],
    { detached: true, shell: true, windowsHide: false }
  );

  await new Promise((resolve) => {
    c.on('close', resolve);
    process.stdout.write("npm completed");
  });

  await insert_env_vars();
}

main().catch((err) => {
  console.error(err)
  process.exit(1)
})
