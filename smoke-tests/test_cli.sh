#!/bin/bash

# uncomment if quiet or die is needed
# source "$prefix/bin/functions.sh"

# ++++++++++++
# ssc --prefix
# ++++++++++++

prefix=$(ssc --prefix 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc --prefix returns 0"
else
  echo "NOT OK ssc --prefix does not return 0"
fi

# ++++++++++++
# ssc --version
# ssc -v
# ++++++++++++

# Run ssc cli with the --version flag and check if it returns a version number
ssc_version=$(ssc --version 2>&1)
ssc_v=$(ssc -v 2>&1)

# `ssc --version` output should be the same as `ssc -v` output
if [ "$ssc_version" == "$ssc_v" ]; then
  echo "OK ssc --version output is the same as ssc -v output"
else
  echo "NOT OK ssc --version output is not the same as ssc -v output"
fi

# ++++++++++++
# ssc
# ssc --help
# ssc -h
# ++++++++++++

ssc_empty=$(ssc 2>&1)
ssc_help=$(ssc --help 2>&1)
ssc_h=$(ssc -h 2>&1)

# `ssc` output should be the same as `ssc --help` output
if [ "$ssc_empty" == "$ssc_help" ]; then
  echo "OK ssc output is the same as ssc --help output"
else
  echo "NOT OK ssc output is not the same as ssc --help output"
fi

# `ssc -h` output should be the same as `ssc --help` output
if [ "$ssc_h" == "$ssc_help" ]; then
  echo "OK ssc -h output is the same as ssc --help output"
else
  echo "NOT OK ssc -h output is not the same as ssc --help output"
fi

# ++++++++++++
# ssc invalid
# ssc --invalid
# ++++++++++++

output=$(ssc invalid 2>&1)
exit_status=$?
if [ $exit_status -eq 1 ]; then
  echo "OK ssc invalid returns 1"
  if [[ "$output" == "• subcommand 'ssc invalid' is not supported"* ]]; then
    echo "OK ssc invalid starts with '• subcommand 'ssc invalid' is not supported'"
  else
    echo "NOT OK ssc invalid output is incorrect"
    echo $output
  fi
else
  echo "NOT OK ssc invalid does not return 1"
fi
# TODO: check that output is • unknown option: --invalid + ssc --help output

output=$(ssc --invalid 2>&1)
exit_status=$?
if [ $exit_status -eq 1 ]; then
  echo "OK ssc --invalid returns 1"
  if [[ "$output" == "• unknown option: --invalid"* ]]; then
    echo "OK ssc --invalid starts with '• unknown option: --invalid'"
  else
    echo "NOT OK ssc --invalid output is incorrect"
    echo $output
  fi
else
  echo "NOT OK ssc --invalid does not return 1"
fi
# TODO: check that output is • unknown option: --invalid + ssc --help output

# ++++++++++++
# ssc env
# ++++++++++++

output=$(ssc env 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc env returns 0"
else
  echo "NOT OK ssc env does not return 0"
fi
# TODO: check that all the lines are present

# ++++++++++++
# ssc list-devices
# ++++++++++++

output=$(ssc list-devices 2>&1)
exit_status=$?
if [ $exit_status -eq 1 ]; then
  echo "OK ssc list-devices returns 1"
  if [[ "$output" == "ERROR: required option '--platform' is missing"* ]]; then
    echo "OK ssc list-devices starts with 'ERROR: required option '--platform' is missing'"
  else
    echo "NOT OK ssc list-devices output is incorrect when run in a directory without a socket.ini file"
    echo $output
  fi
else
  echo "NOT OK ssc list-devices does not return 0"
fi

output=$(ssc list-devices --platform=windows 2>&1)
exit_status=$?
if [ $exit_status -eq 1 ]; then
  echo "OK ssc list-devices returns 1"
  if [[ "$output" == "• list-devices is only supported for iOS devices on macOS."* ]]; then
    echo "OK ssc list-devices starts with '• list-devices is only supported for iOS devices on macOS.' is missing'"
  else
    echo "NOT OK ssc list-devices output is incorrect when run in a directory without a socket.ini file"
    echo $output
  fi
else
  echo "NOT OK ssc list-devices does not return 1"
fi
# TODO: --ecid, --udid, --only

# ++++++++++++
# ssc print-build-dir
# ++++++++++++

output=$(ssc print-build-dir 2>&1)
exit_status=$?
if [ $exit_status -eq 1 ]; then
  echo "OK ssc print-build-dir returns 1"
  if [[ "$output" == "• socket.ini not found in"* ]]; then
    echo "OK ssc print-build-dir starts with '• socket.ini not found' when run in a directory without a socket.ini file"
  else
    echo "NOT OK ssc print-build-dir output is incorrect when run in a directory without a socket.ini file"
    echo $output
  fi
else
  echo "NOT OK ssc print-build-dir does not return 0"
fi

output=$(ssc print-build-dir ./test 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc print-build-dir ./test returns 0"
else
  echo "NOT OK ssc print-build-dir ./test does not return 0"
  echo $output
fi

output=$(ssc print-build-dir ./test --root 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc print-build-dir ./test returns 0"
else
  echo "NOT OK ssc print-build-dir ./test does not return 0"
  echo $output
fi

# FIXME: not implemented
output=$(ssc print-build-dir ./test --prod 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc print-build-dir ./test --prod returns 0"
else
  echo "NOT OK ssc print-build-dir ./test --prod does not return 0"
  echo $output
fi

# FIXME: not implemented. Should return 1 with an error message that --root and --prod cannot be used together
# output=$(ssc print-build-dir ./test --root --prod 2>&1)
# exit_status=$?
# if [ $exit_status -eq 0 ]; then
#   echo "OK ssc print-build-dir ./test returns 0"
# else
#   echo "NOT OK ssc print-build-dir ./test does not return 0"
#   echo $output
# fi

# ++++++++++++
# ssc setup
# ++++++++++++

output=$(ssc setup 2>&1)
exit_status=$?
if [ $exit_status -eq 0 ]; then
  echo "OK ssc setup returns 0"
else
  echo "NOT OK ssc setup does not return 0"
fi
# TODO: mentions --platform=ios. Do we have it?
# TODO: -q, -y flags

# ++++++++++++
# ssc install-app
# ++++++++++++

# TODO

# ++++++++++++
# ssc build
# ++++++++++++

# TODO

# ++++++++++++
# ssc run
# ++++++++++++

# TODO

# ++++++++++++
# ssc init
# ++++++++++++

# TODO: interactive mode
# output=$(ssc init 2>&1)
# exit_status=$?

# TODO: path in a wrong position
# TODO: wrong paths
