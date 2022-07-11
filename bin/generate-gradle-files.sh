#!/usr/bin/env bash
# vim: set syntax=bash:

declare root="$(dirname "$(dirname "${BASH_SOURCE[0]}")")"

rm -f "$root/build.gradle" "$root/gradle.properties"

## build.gradle
cat > "$root/build.gradle" << GRADLE
apply plugin: 'com.android.application'
apply plugin: 'kotlin-android'

android {
  compileSdkVersion 32
  flavorDimensions "default"

  defaultConfig {
    applicationId "__BUNDLE_IDENTIFIER__"
    minSdkVersion 24
    targetSdkVersion 30
    versionCode 1
    versionName "0.0.1"
  }
}

dependencies {
  implementation fileTree(dir: 'libs', include: ['*.jar'])
  implementation "org.jetbrains.kotlin:kotlin-stdlib-jdk7:\$kotlin_version"
  implementation 'androidx.appcompat:appcompat:1.2.0'
}
GRADLE

## gradle.properties
cat > "$root/gradle.properties" << GRADLE
org.gradle.jvmargs=-Xmx2048m
org.gradle.parallel=true

android.useAndroidX=true
android.enableJetifier=true

kotlin.code.style=official
GRADLE
