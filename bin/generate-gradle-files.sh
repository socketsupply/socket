#!/usr/bin/env bash
# vim: set syntax=bash:

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

cd "$root" || exit $?

rm -f "$root/build.gradle" "$root/gradle.properties"

## build.gradle
cat > "$root/build.gradle" << GRADLE
buildscript {
  ext.kotlin_version = '1.9.10'
  repositories {
    google()
    mavenCentral()
  }

  dependencies {
    classpath 'com.android.tools.build:gradle:7.4.1'
    classpath "org.jetbrains.kotlin:kotlin-gradle-plugin:\$kotlin_version"
  }
}

allprojects {
  repositories {
    google()
    mavenCentral()
  }
}

apply plugin: 'com.android.application'
apply plugin: 'kotlin-android'

android {
  compileSdkVersion 34
  ndkVersion "26.0.10792818"
  flavorDimensions "default"

  defaultConfig {
    applicationId "__BUNDLE_IDENTIFIER__"
    minSdkVersion 24
    targetSdkVersion 34
    versionCode 1
    versionName "0.0.1"
  }
}

dependencies {
  implementation "org.jetbrains.kotlin:kotlin-stdlib-jdk7:$kotlin_version"
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:1.73'
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3'
  implementation 'androidx.appcompat:appcompat:1.6.1'
  implementation 'androidx.core:core-ktx:2.2.0'
  implementation 'androidx.webkit:webkit:1.8.0'
}
GRADLE

## gradle.properties
cat > "$root/gradle.properties" << GRADLE
org.gradle.jvmargs=-Xmx2048m
org.gradle.parallel=true

android.useAndroidX=true
android.enableJetifier=true
android.experimental.legacyTransform.forceNonIncremental=true

kotlin.code.style=official
GRADLE

gradle wrapper
