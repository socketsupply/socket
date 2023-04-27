# Configuration

## Section `build`

Key | Default Value | Description
:--- | :--- | :---
copy | "src" |  ssc will copy everything in this directory to the build output directory. This is useful when you want to avoid bundling or want to use tools like vite, webpack, rollup, etc. to build your project and then copy output to the Socket bundle resources directory.
env |  |  An list of environment variables, separated by commas.
flags |  |  Advanced Compiler Settings (ie C++ compiler -02, -03, etc).
headless |  |  If false, the window will never be displayed.
name |  |  The name of the program and executable to be output. Can't contain spaces or special characters. Required field.
output |  |  The binary output path. It's recommended to add this path to .gitignore.

## Section `debug`

Key | Default Value | Description
:--- | :--- | :---
flags |  |  Advanced Compiler Settings for debug purposes (ie C++ compiler -g, etc).

## Section `meta`

Key | Default Value | Description
:--- | :--- | :---
bundle_identifier |  |  A unique ID that identifies the bundle (used by all app stores).
copyright |  |  A string that gets used in the about dialog and package meta info.
description |  |  A short description of the app.
file_limit |  |  Set the limit of files that can be opened by your process.
lang |  |  Localization
maintainer |  |  A String used in the about dialog and meta info.
title |  |  The title of the app used in metadata files. This is NOT a window title. Can contain spaces and special characters. Defaults to name in a [build] section.
version |  |  A string that indicates the version of the application. It should be a semver triple like 1.2.3. Defaults to 1.0.0.

## Section `android`

Key | Default Value | Description
:--- | :--- | :---
aapt_no_compress |  |  Extensions of files that will not be stored compressed in the APK.
enable_standard_ndk_build |  |  Enables gradle based ndk build rather than using external native build (standard ndk is the old slow way)
main_activity |  |  Name of the MainActivity class. Could be overwritten by custom native code.
manifest_permissions |  |  Which permissions does your application need: https://developer.android.com/guide/topics/permissions/overview
native_abis |  |  To restrict the set of ABIs that your application supports, set them here.
native_cflags |  |  Used for adding custom source files and related compiler attributes.
native_sources |  | 
native_makefile |  | 
sources |  | 

## Section `ios`

Key | Default Value | Description
:--- | :--- | :---
codesign_identity |  |  signing guide: https://sockets.sh/guides/#ios-1
distribution_method |  |  Describes how Xcode should export the archive. Available options: app-store, package, ad-hoc, enterprise, development, and developer-id.
provisioning_profile |  |  A path to the provisioning profile used for signing iOS app.
simulator_device |  |  which device to target when building for the simulator

## Section `linux`

Key | Default Value | Description
:--- | :--- | :---
categories |  |  Helps to make your app searchable in Linux desktop environments.
cmd |  |  The command to execute to spawn the "back-end" process.
icon |  |  The icon to use for identifying your app in Linux desktop environments.

## Section `mac`

Key | Default Value | Description
:--- | :--- | :---
appstore_icon |  |  Mac App Store icon
category |  |  A category in the App Store
cmd |  |  The command to execute to spawn the "back-end" process.
icon |  |  The icon to use for identifying your app on MacOS.
sign |  |  TODO Signing guide: https://socketsupply.co/guides/#code-signing-certificates
codesign_identity |  | 
sign_paths |  | 

## Section `native`

Key | Default Value | Description
:--- | :--- | :---
files |  |  Files that should be added to the compile step.
headers |  |  Extra Headers

## Section `win`

Key | Default Value | Description
:--- | :--- | :---
cmd |  |  The command to execute to spawn the “back-end” process.
icon |  |  The icon to use for identifying your app on Windows.
logo |  |  The icon to use for identifying your app on Windows.
pfx |  |  A relative path to the pfx file used for signing.

## Section `window`

Key | Default Value | Description
:--- | :--- | :---
height |  |  The initial height of the first window.
width |  |  The initial width of the first window.

## Section `headless`

Key | Default Value | Description
:--- | :--- | :---
runner |  |  The headless runner command. It is used when no OS specific runner is set.
runner_flags |  |  The headless runner command flags. It is used when no OS specific runner is set.
runner_android |  |  The headless runner command for Android
runner_android_flags |  |  The headless runner command flags for Android
runner_ios |  |  The headless runner command for iOS
runner_ios_flags |  |  The headless runner command flags for iOS
runner_linux |  |  The headless runner command for Linux
runner_linux_flags |  |  The headless runner command flags for Linux
runner_mac |  |  The headless runner command for MacOS
runner_mac_flags |  |  The headless runner command flags for MacOS
runner_win32 |  |  The headless runner command for Windows
runner_win32_flags |  |  The headless runner command flags for Windows

