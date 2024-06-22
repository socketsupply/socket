#!/usr/bin/env bash
# vim: set syntax=bash:

declare root="$(cd "$(dirname "$(dirname "${BASH_SOURCE[0]}")")" && pwd)"

cd "$root" || exit $?

rm -f "$root/build.gradle" "$root/gradle.properties"

## build.gradle
cat > "$root/build.gradle" << GRADLE
buildscript {
  ext.kotlin_version = '1.9.20'
  repositories {
    google()
    mavenCentral()
    maven { url "https://plugins.gradle.org/m2/" }
  }

  dependencies {
    classpath 'com.android.tools.build:gradle:7.4.1'
    classpath "org.jetbrains.kotlin:kotlin-gradle-plugin:\$kotlin_version"
    classpath "org.jetbrains.kotlin:kotlin-android-extensions:\$kotlin_version"
  }
}

allprojects {
  ext.kotlin_version = '1.9.20'
  repositories {
    google()
    mavenCentral()
    maven { url "https://plugins.gradle.org/m2/" }
  }
}

apply plugin: 'org.jetbrains.kotlin.android'
apply plugin: 'com.android.application'
apply plugin: 'kotlin-android-extensions'
apply plugin: 'kotlin-android'

android {
  compileSdkVersion 34
  ndkVersion "26.1.10909125"
  flavorDimensions "default"

  defaultConfig {
    applicationId "socket.runtime"
    minSdkVersion 24
    targetSdkVersion 34
    versionCode 1
    versionName "0.0.1"
  }

  sourceSets {
    main {
      java {
        srcDir 'src'
      }
    }
  }
}

dependencies {
  implementation "org.jetbrains.kotlin:kotlin-stdlib-jdk7:\$kotlin_version"
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3'
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3'
  implementation 'androidx.fragment:fragment-ktx:1.7.1'
  implementation 'androidx.lifecycle:lifecycle-process:2.7.0'
  implementation 'androidx.appcompat:appcompat:1.6.1'
  implementation 'androidx.core:core-ktx:1.13.0'
  implementation 'androidx.webkit:webkit:1.9.0'
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

gradle wrapper && "$root/gradlew" androidDependencies
