//
// Cli Help
//
constexpr auto gHelpText = R"TEXT(
ssc v{{ssc_version}}

usage:
  ssc [SUBCOMMAND] [options] [<project-dir>]
  ssc [SUBCOMMAND] -h

subcommands:
  build                                build project
  list-devices                         get the list of connected devices
  init                                 create a new project (in the current directory)
  install-app                          install app to the device
  print-build-dir                      print build path to stdout
  run                                  run application
  env                                  print relavent environment variables
  setup                                install build dependencies

general options:
  -h, --help                           print help message
  --prefix                             print install path
  -v, --version                        print program version
)TEXT";

constexpr auto gHelpTextBuild = R"TEXT(
ssc v{{ssc_version}}

Build Socket application.

usage:
  ssc build [options] [<project-dir>]

options:
  --platform=<platform>                platform target to build application for (defaults to host):
                                         - android
                                         - android-emulator
                                         - ios
                                         - ios-simulator
  --port=<port>                        load "index.html" from a specific port (if host is not specified, defaults to localhost)
  --host=<host>                        load "index.html" from a specific host (if port is not specified, defaults to 80)
  --test[=path]                        indicate test mode, optionally importing a test file relative to resource files
  --headless                           build application to run in headless mode (without frame or window)
  --prod                               build for production (disables debugging info, inspector, etc.)
  -D, --debug                          enable debug mode
  -E, --env                            add environment variables
  -o, --only-build                     only run build step,
  -p, --package                        package the app for distribution
  -q, --quiet                          hint for less log output
  -r, --run                            run after building
  -V, --verbose                        enable verbose output
  -w, --watch                          watch for changes to rerun build step

Linux options:
  -f, --package-format=<format>        package a Linux application in a specified format for distribution:
                                         - deb (default)
                                         - zip

macOS options:
  -c, --codesign                       code sign application with 'codesign'
  -n, --notarize                       notarize application with 'notarytool'
  -f, --package-format=<format>        package a macOS application in a specified format for distribution:
                                         - zip (default)
                                         - pkg

iOS options:
  -c, --codesign                       code sign application during xcoddbuild
                                       (requires '[ios] provisioning_profile' in 'socket.ini')

Windows options:
  -f, --package-format=<format>        package a Windows application in a specified format for distribution:
                                         - appx (default)
)TEXT";

constexpr auto gHelpTextRun = R"TEXT(
ssc v{{ssc_version}}

Run application.

usage:
  ssc run [options] [<project-dir>]

options:
  --headless                           run application in headless mode (without frame or window)
  --platform=<platform>                platform target to run application on (defaults to host):
                                         - android
                                         - android-emulator
                                         - ios
                                         - ios-simulator
  --port=<port>                        load "index.html" from a specific port (if host is not specified, defaults to localhost)
  --host=<host>                        load "index.html" from a specific host (if port is not specified, defaults to 80)
  --prod                               build for production (disables debugging info, inspector, etc.)
  --test[=path]                        indicate test mode, optionally importing a test file relative to resource files
  -D, --debug                          enable debug mode
  -E, --env                            add environment variables
  -V, --verbose                        enable verbose output
)TEXT";

constexpr auto gHelpTextListDevices = R"TEXT(
ssc v{{ssc_version}}

Get the list of connected devices.

usage:
  ssc list-devices [options] --platform=<platform>

options:
  --platform=<platform>                platform target to list devices for:
                                         - android
                                         - ios
  --ecid                               show device ECID (ios only)
  --udid                               show device UDID (ios only)
  --only                               only show ECID or UDID of the first device (ios only)
)TEXT";

constexpr auto gHelpTextInit = R"TEXT(
ssc v{{ssc_version}}

Create a new project. If the path is not provided, the new project will be created in the current directory.

usage:
  ssc init [<project-dir>]

options:
  -C, --config                         only create the config file
  -n, --name                           project name
)TEXT";

constexpr auto gHelpTextInstallApp = R"TEXT(
ssc v{{ssc_version}}

Install the app to the device or host target.

usage:
  ssc install-app [--platform=<platform>] [--device=<identifier>] [options]

options:
  -D, --debug                          enable debug output
  --device[=identifier]                identifier (ecid, ID) of the device to install to
                                       if not specified, tries to run on the current device
  --platform=<platform>                platform to install application to device (defaults to host)::
                                         - android
                                         - ios
  --prod                               install production application
  -V, --verbose                        enable verbose output

macOS options:
  --target=<target>                    installation target for macOS application (defaults to '/')
                                       the application is installed into '$target/Applications'
)TEXT";

constexpr auto gHelpTextPrintBuildDir = R"TEXT(
ssc v{{ssc_version}}

Create a new project (in the current directory)

usage:
  ssc print-build-dir [--platform=<platform>] [--prod] [--root] [<project-dir>]

options:
  --platform                           platform to print build directory for (defaults to host):
                                         - android
                                         - android-emulator
                                         - ios
                                         - ios-simulator
  --prod                               indicate production build directory
  --root                               print the root build directory
)TEXT";

constexpr auto gHelpTextSetup = R"TEXT(
ssc v{{ssc_version}}

Setup build tools for host or target platform.

Platforms not listed below can be setup using instructions at https://socketsupply.co/guides

usage:
  ssc setup [options] --platform=<platform> [-y|--yes]

options:
  --platform=<platform>                platform target to run setup for (defaults to host):
                                         - android
                                         - ios
  -q, --quiet                          hint for less log output
  -y, --yes                            answer yes to any prompts

)TEXT";

// Validate CSP using Google's CSP Evaluator
// https://csp-evaluator.withgoogle.com
constexpr auto gHelloWorld = R"HTML(
<!doctype html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
    <meta
      http-equiv="Content-Security-Policy"
      content="
        connect-src socket: https: http: blob: ipc: npm: node: wss: ws: ws://localhost:*;
         script-src socket: https: http: blob: npm: node: http://localhost:* 'unsafe-eval' 'unsafe-inline';
         worker-src socket: https: http: blob: 'unsafe-eval' 'unsafe-inline';
          frame-src socket: https: http: blob: http://localhost:*;
            img-src socket: https: http: blob: http://localhost:*;
          child-src socket: https: http: blob:;
         object-src 'none';
      "
    >
    <style type="text/css">
      html, body {
        height: 100%;
      }
      body {
        display: grid;
        justify-content: center;
        align-content: center;
        font-family: helvetica;
        overflow: hidden;
      }
    </style>
    <script src="index.js" type="module"></script>
  </head>
  <body>
    <h1>Hello, World.</h1>
  </body>
</html>
)HTML";

constexpr auto gHelloWorldScript = R"JavaScript(//
// Your JavaScript goes here!
//
import process from 'socket:process'
console.log(`Hello, ${process.platform}!`)
)JavaScript";

//
// macOS 'Info.plist' file
//
constexpr auto gMacOSInfoPList = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <!--- Metadata -->
  <key>CFBundleDisplayName</key>
  <string>{{build_name}}</string>

  <key>CFBundleName</key>
  <string>{{build_name}}</string>

	<key>CFBundleIconFile</key>
	<string>AppIcon</string>

	<key>CFBundleIconName</key>
	<string>AppIcon</string>

  <key>CFBundlePackageType</key>
  <string>APPL</string>

  <key>CFBundleVersion</key>
  <string>{{meta_version}}</string>

  <key>CFBundleShortVersionString</key>
  <string>{{meta_version}}</string>

  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>

  <key>CFBundleExecutable</key>
  <string>{{build_name}}</string>

  <key>CFBundleIdentifier</key>
  <string>{{meta_bundle_identifier}}</string>

  <key>LSUIElement</key>
  <{{application_agent}}/>

  <key>LSApplicationCategoryType</key>
  <string>{{mac_category}}</string>

  <key>NSHumanReadableCopyright</key>
  <string>{{meta_copyright}}</string>

  <key>NSMainNibFile</key>
  <string>MainMenu</string>

  <key>NSPrincipalClass</key>
  <string>NSApplication</string>

  <key>CFBundleURLTypes</key>
  <array>
    <dict>
      <key>CFBundleURLName</key>
      <string>{{meta_application_protocol}}</string>
      <key>CFBundleURLSchemes</key>
      <array>
        <string>{{meta_application_protocol}}</string>
      </array>
    </dict>
  </array>

  <key>NSUserActivityTypes</key>
  <array>
    <string>NSUserActivityTypeBrowsingWeb</string>
  </array>

  <!-- Application configuration -->
  <key>NSLocationDefaultAccuracyReduced</key>
  <true/>

  <key>LSMinimumSystemVersion</key>
  <string>{{mac_minimum_supported_version}}</string>

  <key>LSMultipleInstancesProhibited</key>
  <true/>

  <key>NSHighResolutionCapable</key>
  <true/>

  <key>NSRequiresAquaSystemAppearance</key>
  <false/>

  <key>NSSupportsAutomaticGraphicsSwitching</key>
  <true/>

  <key>SoftResourceLimits</key>
  <dict>
      <key>NumberOfFiles</key>
      <integer>{{meta_file_limit}}</integer>
  </dict>

  <key>WKAppBoundDomains</key>
  <array>
      <string>localhost</string>
      <string>{{meta_bundle_identifier}}</string>
  </array>


  <!-- Permission usage descriptions -->
  <key>NSAppDataUsageDescription</key>
  <string>
    {{meta_title}} would like shared app data access
  </string>

  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>
    {{meta_title}} would like to discover and connect to peers using Bluetooth
  </string>

  <key>NSCameraUsageDescription</key>
  <string>
    {{meta_title}} would like to access to your camera
  </string>

  <key>NSLocationAlwaysUsageDescription</key>
  <string>
    {{meta_title}} would like access to your location
  </string>

  <key>NSLocationWhenInUseUsageDescription</key>
  <string>
    {{meta_title}} would like access to your location when in use
  </string>

  <key>NSLocationTemporaryUsageDescriptionDictionary</key>
  <string>
    {{meta_title}} would like temporary access to your location
  </string>

  <key>NSMicrophoneUsageDescription</key>
  <string>
    {{meta_title}} would like to access to your microphone
  </string>

  <key>NSSpeechRecognitionUsageDescription</key>
  <string>
    {{meta_title}} would like to access Speech Recognition
  </string>

  <key>NSMotionUsageDescription</key>
  <string>
    {{meta_title}} would like to access to detect your device motion
  </string>


  <!-- Security configuration -->
  <key>NSAppTransportSecurity</key>
  <dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>

    <key>NSAllowsLocalNetworking</key>
    <true/>

    <key>NSExceptionDomains</key>
    <dict>
{{macos_app_transport_security_domain_exceptions}}
      <key>127.0.0.1</key>
      <dict>
        <key>NSTemporaryExceptionAllowsInsecureHTTPLoads</key>
        <true/>

        <key>NSTemporaryExceptionRequiresForwardSecrecy</key>
        <false/>

        <key>NSIncludesSubdomains</key>
        <false/>

        <key>NSTemporaryExceptionMinimumTLSVersion</key>
        <string>1.0</string>

        <key>NSTemporaryExceptionAllowsInsecureHTTPSLoads</key>
        <false/>
      </dict>

      <key>localhost</key>
      <dict>
        <key>NSTemporaryExceptionAllowsInsecureHTTPLoads</key>
        <true/>

        <key>NSTemporaryExceptionRequiresForwardSecrecy</key>
        <false/>

        <key>NSIncludesSubdomains</key>
        <false/>

        <key>NSTemporaryExceptionMinimumTLSVersion</key>
        <string>1.0</string>

        <key>NSTemporaryExceptionAllowsInsecureHTTPSLoads</key>
        <false/>
      </dict>
    </dict>
  </dict>


  <!-- Debug information -->
  <key>BuildMachineOSBuild</key>
  <string>{{__xcode_macosx_sdk_build_version}}</string>

  <key>BuildMachineOSBuild</key>
  <string>{{__xcode_macosx_sdk_build_version}}</string>

  <key>DTSDKName</key>
  <string>macosx{{__xcode_macosx_sdk_version}}</string>

  <key>DTXcode</key>
  <string>{{__xcode_version}}</string>

  <key>DTSDKBuild</key>
  <string>{{__xcode_macosx_sdk_version}}</string>

  <key>DTXcodeBuild</key>
  <string>{{__xcode_build_version}}</string>

  <!-- User given plist data -->
{{mac_info_plist_data}}
  </dict>
</plist>
)XML";

// Credits
constexpr auto gCredits = R"HTML(
  <p style="font-family: -apple-system; font-size: small; color: FieldText;">
    Built with ssc v{{ssc_version}}
  </p>
)HTML";

constexpr auto DEFAULT_ANDROID_APPLICATION_NAME = ".App";
constexpr auto DEFAULT_ANDROID_MAIN_ACTIVITY_NAME = ".MainActivity";

//
// Android Manifest
//
constexpr auto gAndroidManifest = R"XML(
<?xml version="1.0" encoding="utf-8"?>
<manifest
  xmlns:android="http://schemas.android.com/apk/res/android"
>
  <uses-sdk
    android:minSdkVersion="34"
    android:targetSdkVersion="34"
  />

  <uses-permission android:name="android.permission.INTERNET" />
  <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
  <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
  {{android_manifest_xml_permissions}}

  <application
    android:name="{{android_application}}"
    android:allowBackup="true"
    android:label="{{meta_title}}"
    android:theme="@style/Theme.AppCompat.DayNight"
    android:supportsRtl="true"
    {{android_application_icon_config}}
    {{android_allow_cleartext}}
  >
    <meta-data
      android:name="android.webkit.WebView.MetricsOptOut"
      android:value="true"
    />
    <activity
      android:name="{{android_main_activity}}"
      android:exported="true"
      android:configChanges="orientation|keyboardHidden|keyboard|screenSize|locale|layoutDirection|fontScale|screenLayout|density|uiMode"
      android:launchMode="singleTop"
      android:enableOnBackInvokedCallback="true"
      android:hardwareAccelerated="true"
    >
      <intent-filter>
        <action android:name="android.intent.action.MAIN" />
        <category android:name="android.intent.category.LAUNCHER" />
      </intent-filter>

      <intent-filter>
        <action android:name="android.intent.action.VIEW" />
        <category android:name="android.intent.category.DEFAULT" />
        <category android:name="android.intent.category.LAUNCHER" />
        <category android:name="android.intent.category.BROWSABLE" />
        <data android:scheme="{{meta_application_protocol}}" />
      </intent-filter>

      {{android_activity_intent_filters}}
    </activity>
  </application>
</manifest>
)XML";

//
// Linux config
//
constexpr auto gDesktopManifest = R"INI(
[Desktop Entry]
Encoding=UTF-8
Version=v{{meta_version}}
Name={{build_name}}
Terminal=false
Type=Application
Exec={{linux_executable_path}} %U
Icon={{linux_icon_path}}
StartupWMClass={{build_name}}
StartupNotify={{linux_desktop_startup_notify}}
Comment={{meta_description}}
Categories={{linux_categories}};
MimeType=x-scheme-handler/{{meta_application_protocol}}
)INI";

constexpr auto gDebianManifest = R"DEB(Package: {{build_name}}
Version: {{meta_version}}
Architecture: {{arch}}
Maintainer: {{meta_maintainer}}
Description: {{meta_title}}
 {{meta_description}}
)DEB";

//
// Windows config
//
constexpr auto gWindowsAppManifest = R"XML(<?xml version="1.0" encoding="utf-8"?>
<Package
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
  xmlns:uap3="http://schemas.microsoft.com/appx/manifest/uap/windows10/3"
  xmlns:desktop="http://schemas.microsoft.com/appx/manifest/desktop/windows10"
  xmlns:desktop2="http://schemas.microsoft.com/appx/manifest/desktop/windows10/2"
  xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
  IgnorableNamespaces="uap3 rescap"
>
  <Identity Name="{{meta_bundle_identifier}}"
    ProcessorArchitecture="neutral"
    Publisher="{{win_publisher}}"
    Version="{{win_version}}"
    ResourceId="{{build_name}}"
  />

  <Properties>
    <DisplayName>{{meta_title}}</DisplayName>
    <Description>{{meta_description}}</Description>
    <Logo>{{win_logo}}</Logo>
    <PublisherDisplayName>{{meta_maintainer}}</PublisherDisplayName>
  </Properties>

  <Resources>
    <Resource Language="{{meta_lang}}"/>
  </Resources>

  <Dependencies>
    <TargetDeviceFamily
      Name="Windows.Desktop"
      MinVersion="10.0.17763.0"
      MaxVersionTested="10.0.19041.1"
    />
  </Dependencies>

  <Capabilities>
    <rescap:Capability Name="runFullTrust" />
  </Capabilities>
  <Applications>
    <Application
      Id="{{build_name}}"
      EntryPoint="Windows.FullTrustApplication"
      Executable="{{win_exe}}"
    >
      <uap:VisualElements DisplayName="{{meta_title}}"
        Square150x150Logo="{{win_logo}}"
        Square44x44Logo="{{win_logo}}"
        Description="{{meta_description}}"
        BackgroundColor="#20123A"
      >
        <uap:DefaultTile Wide310x150Logo="{{win_logo}}" />
      </uap:VisualElements>
      <Extensions>
        <uap3:Extension
          Category="windows.appExecutionAlias"
          EntryPoint="Windows.FullTrustApplication"
          Executable="{{win_exe}}"
        >
          <uap3:AppExecutionAlias>
            <desktop:ExecutionAlias Alias="{{win_exe}}" />
          </uap3:AppExecutionAlias>
        </uap3:Extension>
      </Extensions>
    </Application>

  </Applications>
  <Extensions>
    <desktop2:Extension Category="windows.firewallRules">
      <desktop2:FirewallRules Executable="{{win_exe}}">
        <desktop2:Rule
          Direction="in"
          IPProtocol="TCP"
          LocalPortMin="1"
          LocalPortMax="65535"
          RemotePortMin="1"
          RemotePortMax="65535"
          Profile="all" />
        <desktop2:Rule
          Direction="out"
          IPProtocol="TCP"
          LocalPortMin="1"
          LocalPortMax="65535"
          RemotePortMin="1"
          RemotePortMax="65535"
          Profile="all" />

        <desktop2:Rule
          Direction="in"
          IPProtocol="UDP"
          LocalPortMin="1"
          LocalPortMax="65535"
          RemotePortMin="1"
          RemotePortMax="65535"
          Profile="all" />
        <desktop2:Rule
          Direction="out"
          IPProtocol="UDP"
          LocalPortMin="1"
          LocalPortMax="65535"
          RemotePortMin="1"
          RemotePortMax="65535"
          Profile="all" />
      </desktop2:FirewallRules>
    </desktop2:Extension>
  </Extensions>
</Package>
)XML";

//
// Template generated by XCode.
//
constexpr auto gXCodeProject = R"ASCII(// !$*UTF8*$!
{
  archiveVersion = 1;
  classes = {
  };
  objectVersion = 55;
  objects = {

/* Begin PBXBuildFile section */
    {{__ios_native_extensions_build_context_sections}}
		171C1C2B2AC38A70005F587F /* CoreLocation.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 171C1C2A2AC38A70005F587F /* CoreLocation.framework */; };
		179989D22A867B260041EDC1 /* UniformTypeIdentifiers.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 179989D12A867B260041EDC1 /* UniformTypeIdentifiers.framework */; };
		17A7F8F229358D220051D146 /* init.cc in Sources */ = {isa = PBXBuildFile; fileRef = 17A7F8EE29358D180051D146 /* init.cc */; };
		17A7F8F529358D430051D146 /* libsocket-runtime.a in Frameworks */ = {isa = PBXBuildFile; fileRef = 17A7F8F329358D430051D146 /* libsocket-runtime.a */; };
		17A7F8F629358D430051D146 /* libuv.a in Frameworks */ = {isa = PBXBuildFile; fileRef = 17A7F8F429358D430051D146 /* libuv.a */; };
		17A7F8F629358D4A0051D146 /* libllama.a in Frameworks */ = {isa = PBXBuildFile; fileRef = 17A7F8F429358D430051D147 /* libllama.a */; };
		17A7F8F729358D4D0051D146 /* main.o in Frameworks */ = {isa = PBXBuildFile; fileRef = 17A7F8F129358D180051D146 /* main.o */; };
		17C230BA28E9398700301440 /* Foundation.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 17C230B928E9398700301440 /* Foundation.framework */; };
		290F7EBF2768C49000486988 /* UIKit.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 294A3C792763E9C6007B5B9A /* UIKit.framework */; };
		290F7F87276BC2B000486988 /* lib in Resources */ = {isa = PBXBuildFile; fileRef = 290F7F86276BC2B000486988 /* lib */; };
		29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */ = {isa = PBXBuildFile; fileRef = 29124C5B2761336B001832A0 /* LaunchScreen.storyboard */; };
		294A3C852764EAB7007B5B9A /* ui in Resources */ = {isa = PBXBuildFile; fileRef = 294A3C842764EAB7007B5B9A /* ui */; };
    034B592125768A7B005D0134 /* default.metallib in Resources */ = {isa = PBXBuildFile; fileRef = 034B592025768A7B005D0134 /* default.metallib */; };
		294A3CA02768C429007B5B9A /* WebKit.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 294A3C7B2763EA7F007B5B9A /* WebKit.framework */; };
		2996EDB22770BC1F00C672A0 /* Accelerate.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672B1 /* Accelerate.framework */; };
		2996EDB22770BC1F00C672A1 /* Metal.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672A1 /* Metal.framework */; };
		2996EDB22770BC1F00C672A2 /* Network.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672A2 /* Network.framework */; };
		2996EDB22770BC1F00C672A3 /* CoreBluetooth.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672A3 /* CoreBluetooth.framework */; };
		2996EDB22770BC1F00C672A4 /* UserNotifications.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672A4 /* UserNotifications.framework */; };
		2996EDB22770BC1F00C672B0 /* QuartzCore.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 2996EDB12770BC1F00C672B0 /* QuartzCore.framework */; };
		2996EDB22770BC1F00C672A5 /* Assets.xcassets in Resources */ = {isa = PBXBuildFile; fileRef = 29124C5E2761336B001832A1 /* Assets.xcassets */; };
/* End PBXBuildFile section */

/* Begin PBXFileReference section */
    {{__ios_native_extensions_build_context_refs}}
		171C1C2A2AC38A70005F587F /* CoreLocation.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = CoreLocation.framework; path = System/Library/Frameworks/CoreLocation.framework; sourceTree = SDKROOT; };
		179989D12A867B260041EDC1 /* UniformTypeIdentifiers.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = UniformTypeIdentifiers.framework; path = System/Library/Frameworks/UniformTypeIdentifiers.framework; sourceTree = SDKROOT; };
		17A7F8EE29358D180051D146 /* init.cc */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.objcpp; path = init.cc; sourceTree = "<group>"; };
		17A7F8F129358D180051D146 /* main.o */ = {isa = PBXFileReference; lastKnownFileType = "compiled.mach-o.objfile"; path = main.o; sourceTree = "<group>"; };
		17A7F8F329358D430051D146 /* libsocket-runtime.a */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = "libsocket-runtime.a"; path = "lib/libsocket-runtime.a"; sourceTree = "<group>"; };
		17A7F8F429358D430051D146 /* libuv.a */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = libuv.a; path = lib/libuv.a; sourceTree = "<group>"; };
		17A7F8F429358D430051D147 /* libllama.a */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = libllama.a; path = lib/libllama.a; sourceTree = "<group>"; };
		17C230B928E9398700301440 /* Foundation.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = Foundation.framework; path = System/Library/Frameworks/Foundation.framework; sourceTree = SDKROOT; };
		17E73FEE28FCD3360087604F /* libuv-ios.a */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = "libuv-ios.a"; path = "lib/libuv-ios.a"; sourceTree = "<group>"; };
		290F7F86276BC2B000486988 /* lib */ = {isa = PBXFileReference; lastKnownFileType = folder; path = lib; sourceTree = "<group>"; };
		29124C4A27613369001832A0 /* {{build_name}}.app */ = {isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = "{{build_name}}.app"; sourceTree = BUILT_PRODUCTS_DIR; };
		29124C5C2761336B001832A0 /* Base */ = {isa = PBXFileReference; lastKnownFileType = file.storyboard; name = Base; path = Base.lproj/LaunchScreen.storyboard; sourceTree = "<group>"; };
		29124C5E2761336B001832A0 /* Info.plist */ = {isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; };
    034B592025768A7B005D0134 /* default.metallib */ = {isa = PBXFileReference; lastKnownFileType = "archive.metal-library"; path = "lib/default.metallib"; sourceTree = "<group>"; };
		29124C5E2761336B001832A1 /* Assets.xcassets */ = {isa = PBXFileReference; lastKnownFileType = folder.assetcatalog; path = Assets.xcassets; sourceTree = "<group>"; };
		294A3C792763E9C6007B5B9A /* UIKit.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = UIKit.framework; path = System/Library/Frameworks/UIKit.framework; sourceTree = SDKROOT; };
		294A3C7B2763EA7F007B5B9A /* WebKit.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = WebKit.framework; path = System/Library/Frameworks/WebKit.framework; sourceTree = SDKROOT; };
		294A3C842764EAB7007B5B9A /* ui */ = {isa = PBXFileReference; lastKnownFileType = folder; path = ui; sourceTree = "<group>"; };
		294A3C9027677424007B5B9A /* socket.entitlements */ = {isa = PBXFileReference; lastKnownFileType = text.plist.entitlements; path = socket.entitlements; sourceTree = "<group>"; };
    2996EDB12770BC1F00C672B1 /* Accelerate.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = Accelerate.framework; path = System/Library/Frameworks/Accelerate.framework; sourceTree = SDKROOT; };
    2996EDB12770BC1F00C672A1 /* Metal.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = Metal.framework; path = System/Library/Frameworks/Metal.framework; sourceTree = SDKROOT; };
		2996EDB12770BC1F00C672A2 /* Network.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = Network.framework; path = System/Library/Frameworks/Network.framework; sourceTree = SDKROOT; };
		2996EDB12770BC1F00C672A3 /* CoreBluetooth.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = CoreBluetooth.framework; path = System/Library/Frameworks/CoreBluetooth.framework; sourceTree = SDKROOT; };
		2996EDB12770BC1F00C672A4 /* UserNotifications.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = UserNotifications.framework; path = System/Library/Frameworks/UserNotifications.framework; sourceTree = SDKROOT; };
		2996EDB12770BC1F00C672B0 /* QuartzCore.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = Quartzcore.framework; path = System/Library/Frameworks/Quartzcore.framework; sourceTree = SDKROOT; };
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		29124C4727613369001832A0 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
        {{__ios_native_extensions_build_ids}}
				171C1C2B2AC38A70005F587F /* CoreLocation.framework in Frameworks */,
				179989D22A867B260041EDC1 /* UniformTypeIdentifiers.framework in Frameworks */,
				17A7F8F529358D430051D146 /* libsocket-runtime.a in Frameworks */,
				17A7F8F629358D430051D146 /* libuv.a in Frameworks */,
        17A7F8F629358D4A0051D146 /* libllama.a in Frameworks */,
				17A7F8F729358D4D0051D146 /* main.o in Frameworks */,
				17C230BA28E9398700301440 /* Foundation.framework in Frameworks */,
				2996EDB22770BC1F00C672A0 /* Accelerate.framework in Frameworks */,
				2996EDB22770BC1F00C672A1 /* Metal.framework in Frameworks */,
				2996EDB22770BC1F00C672A2 /* Network.framework in Frameworks */,
				2996EDB22770BC1F00C672A3 /* CoreBluetooth.framework in Frameworks */,
				2996EDB22770BC1F00C672A4 /* UserNotifications.framework in Frameworks */,
        2996EDB22770BC1F00C672B0 /* QuartzCore.framework in Frameworks */,
				294A3CA02768C429007B5B9A /* WebKit.framework in Frameworks */,
				290F7EBF2768C49000486988 /* UIKit.framework in Frameworks */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		1790CE4C2AD78CCF00AA7E5B /* core */ = {
			isa = PBXGroup;
			children = (
			);
			path = core;
			sourceTree = "<group>";
		};
		17A7F8EF29358D180051D146 /* objects */ = {
			isa = PBXGroup;
			children = (
				17A7F8F029358D180051D146 /* ios */,
			);
			path = objects;
			sourceTree = "<group>";
		};
		17A7F8F029358D180051D146 /* ios */ = {
			isa = PBXGroup;
			children = (
				17A7F8F129358D180051D146 /* main.o */,
			);
			path = ios;
			sourceTree = "<group>";
		};
		29124C4127613369001832A0 = {
			isa = PBXGroup;
			children = (
				1790CE512AD792B600AA7E5B /* core */,
				17A7F8EE29358D180051D146 /* init.cc */,
				17A7F8EF29358D180051D146 /* objects */,
				290F7F86276BC2B000486988 /* lib */,
				294A3C9027677424007B5B9A /* socket.entitlements */,
				294A3C842764EAB7007B5B9A /* ui */,
				29124C5B2761336B001832A0 /* LaunchScreen.storyboard */,
				29124C5E2761336B001832A0 /* Info.plist */,
				29124C5E2761336B001832A1 /* Assets.xcassets */,
				29124C4B27613369001832A0 /* Products */,
				294A3C782763E9C6007B5B9A /* Frameworks */,
			);
			sourceTree = "<group>";
		};
		29124C4B27613369001832A0 /* Products */ = {
			isa = PBXGroup;
			children = (
				29124C4A27613369001832A0 /* {{build_name}}.app */,
			);
			name = Products;
			sourceTree = "<group>";
		};
		294A3C782763E9C6007B5B9A /* Frameworks */ = {
			isa = PBXGroup;
			children = (
        {{__ios_native_extensions_build_refs}}
				171C1C2A2AC38A70005F587F /* CoreLocation.framework */,
				179989D12A867B260041EDC1 /* UniformTypeIdentifiers.framework */,
				17A7F8F329358D430051D146 /* libsocket-runtime.a */,
				17A7F8F429358D430051D146 /* libuv.a */,
        17A7F8F429358D430051D147 /* libllama.a */,
				17E73FEE28FCD3360087604F /* libuv-ios.a */,
				17C230B928E9398700301440 /* Foundation.framework */,
        2996EDB12770BC1F00C672A1 /* Metal.framework */,
        2996EDB12770BC1F00C672B1 /* Accelerate.framework */,
				2996EDB12770BC1F00C672A2 /* Network.framework */,
				2996EDB12770BC1F00C672A3 /* CoreBluetooth.framework */,
				2996EDB12770BC1F00C672A4 /* UserNotifications.framework */,
				2996EDB12770BC1F00C672B0 /* QuartzCore.framework */,
				294A3C7B2763EA7F007B5B9A /* WebKit.framework */,
				294A3C792763E9C6007B5B9A /* UIKit.framework */,
			);
			name = Frameworks;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
		29124C4927613369001832A0 /* {{build_name}} */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget {{build_name}} */;
			buildPhases = (
				29124C4627613369001832A0 /* Sources */,
				29124C4727613369001832A0 /* Frameworks */,
				29124C4827613369001832A0 /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = "{{build_name}}";
			productName = "{{build_name}}";
			productReference = 29124C4A27613369001832A0 /* {{build_name}}.app */;
			productType = "com.apple.product-type.application";
		};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		29124C4227613369001832A0 /* Project object */ = {
			isa = PBXProject;
			attributes = {
				BuildIndependentTargetsInParallel = 1;
				LastUpgradeCheck = 1340;
				TargetAttributes = {
					29124C4927613369001832A0 = {
						CreatedOnToolsVersion = 13.1;
					};
				};
			};
			buildConfigurationList = 29124C4527613369001832A0 /* Build configuration list for PBXProject "{{build_name}}" */;
			compatibilityVersion = "Xcode 13.0";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = 29124C4127613369001832A0;
			productRefGroup = 29124C4B27613369001832A0 /* Products */;
			projectDirPath = "";
			projectRoot = "";
			targets = (
				29124C4927613369001832A0 /* {{build_name}} */,
			);
		};
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
		29124C4827613369001832A0 /* Resources */ = {
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */,
				294A3C852764EAB7007B5B9A /* ui in Resources */,
				034B592125768A7B005D0134 /* default.metallib in Resources */,
				2996EDB22770BC1F00C672A5 /* Assets.xcassets in Resources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
		29124C4627613369001832A0 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				17A7F8F229358D220051D146 /* init.cc in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

/* Begin PBXVariantGroup section */
		29124C5B2761336B001832A0 /* LaunchScreen.storyboard */ = {
			isa = PBXVariantGroup;
			children = (
				29124C5C2761336B001832A0 /* Base */,
			);
			name = LaunchScreen.storyboard;
			sourceTree = "<group>";
		};
/* End PBXVariantGroup section */

/* Begin XCBuildConfiguration section */
    29124C772761336B001832A0 /* Debug */ = {
      isa = XCBuildConfiguration;
      buildSettings = {
        ALWAYS_SEARCH_USER_PATHS = NO;
        CLANG_ANALYZER_NONNULL = NO;
        CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
        CLANG_CXX_LANGUAGE_STANDARD = "c++20";
        CLANG_CXX_LIBRARY = "libc++";
        CLANG_ENABLE_MODULES = YES;
        CLANG_ENABLE_OBJC_ARC = YES;
        CLANG_ENABLE_OBJC_WEAK = YES;
        CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
        CLANG_WARN_BOOL_CONVERSION = YES;
        CLANG_WARN_COMMA = YES;
        CLANG_WARN_CONSTANT_CONVERSION = YES;
        CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
        CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
        CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
        CLANG_WARN_EMPTY_BODY = YES;
        CLANG_WARN_ENUM_CONVERSION = YES;
        CLANG_WARN_INFINITE_RECURSION = YES;
        CLANG_WARN_INT_CONVERSION = YES;
        CLANG_WARN_NON_LITERAL_NULL_CONVERSION = NO;
        CLANG_WARN_NULLABLE_TO_NONNULL_CONVERSION = NO;
        CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
        CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
        CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
        CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
        CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
        CLANG_WARN_STRICT_PROTOTYPES = YES;
        CLANG_WARN_SUSPICIOUS_MOVE = YES;
        CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
        CLANG_WARN_UNREACHABLE_CODE = YES;
        CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
        COPY_PHASE_STRIP = NO;
        DEBUG_INFORMATION_FORMAT = dwarf;
        ENABLE_STRICT_OBJC_MSGSEND = YES;
        ENABLE_TESTABILITY = YES;
        GCC_C_LANGUAGE_STANDARD = gnu11;
        GCC_DYNAMIC_NO_PIC = NO;
        GCC_NO_COMMON_BLOCKS = YES;
        GCC_OPTIMIZATION_LEVEL = 0;
        GCC_PREPROCESSOR_DEFINITIONS = (
          "DEBUG=1",
          "SOCKET_RUNTIME_VERSION={{SOCKET_RUNTIME_VERSION}}",
          "SOCKET_RUNTIME_VERSION_HASH={{SOCKET_RUNTIME_VERSION_HASH}}",
          "SOCKET_RUNTIME_PLATFORM_SANDBOXED={{SOCKET_RUNTIME_PLATFORM_SANDBOXED}}",
          "$(inherited)",
        );
        GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
        GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
        GCC_WARN_UNDECLARED_SELECTOR = YES;
        GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
        GCC_WARN_UNUSED_FUNCTION = YES;
        GCC_WARN_UNUSED_VARIABLE = YES;
        IPHONEOS_DEPLOYMENT_TARGET = {{ios_deployment_target}};
        MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
        MTL_FAST_MATH = YES;
        ONLY_ACTIVE_ARCH = YES;
        SDKROOT = {{ios_sdkroot}};
        SUPPORTED_PLATFORMS = "iphonesimulator iphoneos";
      };
      name = Debug;
    };
    29124C782761336B001832A0 /* Release */ = {
      isa = XCBuildConfiguration;
      buildSettings = {
        ALWAYS_SEARCH_USER_PATHS = NO;
        CLANG_ANALYZER_NONNULL = NO;
        CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
        CLANG_CXX_LANGUAGE_STANDARD = "c++20";
        CLANG_CXX_LIBRARY = "libc++";
        CLANG_ENABLE_MODULES = YES;
        CLANG_ENABLE_OBJC_ARC = YES;
        CLANG_ENABLE_OBJC_WEAK = YES;
        CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
        CLANG_WARN_BOOL_CONVERSION = YES;
        CLANG_WARN_COMMA = YES;
        CLANG_WARN_CONSTANT_CONVERSION = YES;
        CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
        CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
        CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
        CLANG_WARN_EMPTY_BODY = YES;
        CLANG_WARN_ENUM_CONVERSION = YES;
        CLANG_WARN_INFINITE_RECURSION = YES;
        CLANG_WARN_INT_CONVERSION = YES;
        CLANG_WARN_NON_LITERAL_NULL_CONVERSION = NO;
        CLANG_WARN_NULLABLE_TO_NONNULL_CONVERSION = NO;
        CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
        CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
        CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
        CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
        CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
        CLANG_WARN_STRICT_PROTOTYPES = YES;
        CLANG_WARN_SUSPICIOUS_MOVE = YES;
        CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
        CLANG_WARN_UNREACHABLE_CODE = YES;
        CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
        COPY_PHASE_STRIP = NO;
        DEBUG_INFORMATION_FORMAT = "dwarf-with-dsym";
        ENABLE_NS_ASSERTIONS = NO;
        ENABLE_STRICT_OBJC_MSGSEND = YES;
        GCC_C_LANGUAGE_STANDARD = gnu11;
        GCC_NO_COMMON_BLOCKS = YES;
        GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
        GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
        GCC_WARN_UNDECLARED_SELECTOR = YES;
        GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
        GCC_WARN_UNUSED_FUNCTION = YES;
        GCC_WARN_UNUSED_VARIABLE = YES;
        GCC_PREPROCESSOR_DEFINITIONS = (
          "SOCKET_RUNTIME_VERSION={{SOCKET_RUNTIME_VERSION}}",
          "SOCKET_RUNTIME_VERSION_HASH={{SOCKET_RUNTIME_VERSION_HASH}}",
          "$(inherited)",
        );
        IPHONEOS_DEPLOYMENT_TARGET = {{ios_deployment_target}};
        MTL_ENABLE_DEBUG_INFO = NO;
        MTL_FAST_MATH = YES;
        SDKROOT = {{ios_sdkroot}};
        SUPPORTED_PLATFORMS = "iphonesimulator iphoneos";
        VALIDATE_PRODUCT = YES;
      };
      name = Release;
    };
    29124C7A2761336B001832A0 /* Debug */ = {
      isa = XCBuildConfiguration;
      buildSettings = {
        ARCHS = "$(ARCHS_STANDARD)";
        ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
        ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
        CODE_SIGN_ENTITLEMENTS = "$(PROJECT_DIR)/socket.entitlements";
        CODE_SIGN_IDENTITY = "{{ios_codesign_identity}}";
        CODE_SIGN_STYLE = Manual;
        CURRENT_PROJECT_VERSION = {{ios_project_version}};
        DEVELOPMENT_TEAM = "{{apple_team_identifier}}";
        ENABLE_BITCODE = NO;
        GENERATE_INFOPLIST_FILE = YES;
        HEADER_SEARCH_PATHS = "$(PROJECT_DIR)/include";
        INFOPLIST_FILE = Info.plist;
        INFOPLIST_KEY_NSHumanReadableCopyright = "{{meta_copyright}}";
        INFOPLIST_KEY_UILaunchStoryboardName = LaunchScreen;
        INFOPLIST_KEY_UIRequiresFullScreen = YES;
        INFOPLIST_KEY_UIStatusBarHidden = YES;
        INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        LD_RUNPATH_SEARCH_PATHS = (
          "$(inherited)",
          "@executable_path/Frameworks",
          "@executable_path/../Frameworks",
        );
        LIBRARY_SEARCH_PATHS = "$(PROJECT_DIR)/lib";
        MARKETING_VERSION = {{meta_version}};
        ONLY_ACTIVE_ARCH = YES;
        OTHER_CFLAGS = (
          "-DHOST=\\\"{{host}}\\\"",
          "-DPORT={{port}}",
        );
        PRODUCT_BUNDLE_IDENTIFIER = "{{meta_bundle_identifier}}";
        PRODUCT_NAME = "$(TARGET_NAME)";
        PROVISIONING_PROFILE_SPECIFIER = "{{ios_provisioning_specifier}}";
        SWIFT_EMIT_LOC_STRINGS = YES;
        TARGETED_DEVICE_FAMILY = "1,2";
        WARNING_CFLAGS = (
          "$(inherited)",
          "-Wno-nullability-completeness",
        );
      };
      name = Debug;
    };
    29124C7B2761336B001832A0 /* Release */ = {
      isa = XCBuildConfiguration;
      buildSettings = {
        ARCHS = "$(ARCHS_STANDARD)";
        ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
        ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
        CODE_SIGN_ENTITLEMENTS = "$(PROJECT_DIR)/socket.entitlements";
        CODE_SIGN_IDENTITY = "iPhone Distribution";
        CODE_SIGN_STYLE = Manual;
        CURRENT_PROJECT_VERSION = {{ios_project_version}};
        DEVELOPMENT_TEAM = "{{apple_team_identifier}}";
        ENABLE_BITCODE = NO;
        GENERATE_INFOPLIST_FILE = YES;
        HEADER_SEARCH_PATHS = "$(PROJECT_DIR)/include";
        INFOPLIST_FILE = Info.plist;
        INFOPLIST_KEY_NSHumanReadableCopyright = "{{meta_copyright}}";
        INFOPLIST_KEY_UILaunchStoryboardName = LaunchScreen;
        INFOPLIST_KEY_UIRequiresFullScreen = YES;
        INFOPLIST_KEY_UIStatusBarHidden = YES;
        INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
        LD_RUNPATH_SEARCH_PATHS = (
          "$(inherited)",
          "@executable_path/Frameworks",
          "@executable_path/../Frameworks",
        );
        LIBRARY_SEARCH_PATHS = "$(PROJECT_DIR)/lib";
        MARKETING_VERSION = {{meta_version}};
        ONLY_ACTIVE_ARCH = YES;
        PRODUCT_BUNDLE_IDENTIFIER = "{{meta_bundle_identifier}}";
        PRODUCT_NAME = "$(TARGET_NAME)";
        PROVISIONING_PROFILE_SPECIFIER = "{{ios_provisioning_specifier}}";
        SWIFT_EMIT_LOC_STRINGS = YES;
        TARGETED_DEVICE_FAMILY = "1,2";
        WARNING_CFLAGS = (
          "$(inherited)",
          "-Wno-nullability-completeness",
        );
      };
      name = Release;
    };
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
    29124C4527613369001832A0 /* Build configuration list for PBXProject "{{build_name}}" */ = {
      isa = XCConfigurationList;
      buildConfigurations = (
        29124C772761336B001832A0 /* Debug */,
        29124C782761336B001832A0 /* Release */,
      );
      defaultConfigurationIsVisible = 0;
      defaultConfigurationName = Release;
    };
    29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget "{{build_name}}" */ = {
      isa = XCConfigurationList;
      buildConfigurations = (
        29124C7A2761336B001832A0 /* Debug */,
        29124C7B2761336B001832A0 /* Release */,
      );
      defaultConfigurationIsVisible = 0;
      defaultConfigurationName = Release;
    };
/* End XCConfigurationList section */
  };
  rootObject = 29124C4227613369001832A0 /* Project object */;

})ASCII";

//
// Template generated by XCode.
//
constexpr auto gXCodeExportOptions = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>method</key>
  <string>{{ios_distribution_method}}</string>
  <key>teamID</key>
  <string>{{apple_team_identifier}}</string>
  <key>uploadBitcode</key>
  <true/>
  <key>compileBitcode</key>
  <true/>
  <key>uploadSymbols</key>
  <true/>
  <key>signingStyle</key>
  <string>manual</string>
  <key>signingCertificate</key>
  <string>{{ios_codesign_identity}}</string>
  <key>provisioningProfiles</key>
  <dict>
    <key>{{meta_bundle_identifier}}</key>
    <string>{{ios_provisioning_profile}}</string>
  </dict>
</dict>
</plist>)XML";

//
// iOS 'Info.plist' file
//
constexpr auto gIOSInfoPList = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <!--- Metadata -->
  <key>CFBundleIdentifier</key>
  <string>{{meta_bundle_identifier}}</string>

  <key>CFBundleDisplayName</key>
  <string>{{build_name}}</string>

  <key>CFBundleName</key>
  <string>{{build_name}}</string>

	<key>CFBundleIconFile</key>
	<string>AppIcon</string>

	<key>CFBundleIconName</key>
	<string>AppIcon</string>

  <key>compileBitcode</key>
  <{{meta_compile_bitcode}}/>

  <key>uploadBitcode</key>
  <{{meta_upload_bitcode}}/>

  <key>uploadSymbols</key>
  <{{meta_upload_symbols}}/>

  <key>CFBundleURLTypes</key>
  <array>
    <dict>
      <key>CFBundleURLName</key>
      <string>{{meta_application_protocol}}</string>
      <key>CFBundleURLSchemes</key>
      <array>
        <string>{{meta_application_protocol}}</string>
      </array>
    </dict>
  </array>

  <key>LSApplicationCategoryType</key>
  <string>{{ios_category}}</string>

  <!-- Application configuration -->
  <key>LSApplicationQueriesSchemes</key>
  <array>
    <string>{{ios_protocol}}</string>
  </array>

  <key>NSHighResolutionCapable</key>
  <true/>

  <key>NSLocationDefaultAccuracyReduced</key>
  <false/>

  <key>NSRequiresAquaSystemAppearance</key>
  <false/>

  <key>NSSupportsAutomaticGraphicsSwitching</key>
  <true/>

  <key>ITSAppUsesNonExemptEncryption</key>
  <{{ios_nonexempt_encryption}}/>

  <!-- Permission usage descriptions -->
  <key>NSAppDataUsageDescription</key>
  <string>
    {{meta_title}} would like shared app data access
  </string>

  <key>NSBluetoothAlwaysUsageDescription</key>
  <string>
    {{meta_title}} would like to discover and connect to peers using Bluetooth
  </string>

  <key>NSCameraUsageDescription</key>
  <string>
    {{meta_title}} would like to access to your camera
  </string>

  <key>NSLocalNetworkUsageDescription</key>
  <string>
    {{meta_title}} would like to discover and connect to peers using your local network
  </string>

  <key>NSLocationAlwaysUsageDescription</key>
  <string>
    {{meta_title}} would like access to your location
  </string>

  <key>NSLocationWhenInUseUsageDescription</key>
  <string>
    {{meta_title}} would like access to your location when in use
  </string>

  <key>NSLocationAlwaysAndWhenInUseUsageDescription</key>
  <string>
    {{meta_title}} would like access to your location
  </string>

  <key>NSLocationTemporaryUsageDescriptionDictionary</key>
  <string>
    {{meta_title}} would like temporary access to your location
  </string>

  <key>NSMicrophoneUsageDescription</key>
  <string>
    {{meta_title}} would like to access to your microphone
  </string>

  <key>NSSpeechRecognitionUsageDescription</key>
  <string>
    {{meta_title}} would like to access Speech Recognition
  </string>

  <key>NSMotionUsageDescription</key>
  <string>
    {{meta_title}} would like to access to detect your device motion
  </string>


  <!-- Security configuration -->
  <key>NSAppTransportSecurity</key>
  <dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
  </dict>


  <key>UIApplicationSupportsIndirectInputEvents</key>
  <true/>

  <!-- User given plist data -->
{{ios_info_plist_data}}
</dict>
</plist>)XML";

constexpr auto gXcodeEntitlements = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <!-- Generated entitlements given plist data -->
{{configured_entitlements}}
</dict>
</plist>)XML";

//
// Android top level `build.gradle` in Groovy Syntax
//
constexpr auto gGradleBuild = R"GROOVY(
buildscript {
  ext.kotlin_version = '1.9.20'
  repositories {
    google()
    mavenCentral()
  }

  dependencies {
    classpath 'com.android.tools.build:gradle:8.2.0'
    classpath "org.jetbrains.kotlin:kotlin-gradle-plugin:$kotlin_version"
  }
}

allprojects {
  repositories {
    google()
    mavenCentral()
  }
}

task clean (type: Delete) {
  delete rootProject.buildDir
}
)GROOVY";

//
// Android source `build.gradle` in Groovy Syntax
//
constexpr auto gGradleBuildForSource = R"GROOVY(
apply plugin: 'com.android.application'
apply plugin: 'kotlin-android'

android {
  compileSdkVersion 34
  ndkVersion "27.2.12479018"
  flavorDimensions "default"
  namespace '{{android_bundle_identifier}}'

  compileOptions {
    sourceCompatibility = JavaVersion.VERSION_20
    targetCompatibility = JavaVersion.VERSION_20
  }

  buildFeatures {
    buildConfig = true
  }

  kotlinOptions {
    jvmTarget = 20
  }

  defaultConfig {
    applicationId "{{android_bundle_identifier}}"
    minSdkVersion 34
    targetSdkVersion 34
    versionCode {{meta_revision}}
    versionName "{{meta_version}}"

    ndk {
      abiFilters {{android_ndk_abi_filters}}
    }

{{android_default_config_external_native_build}}
  }

  signingConfigs {
    release {
{{android_signingconfig_release}}
      v1SigningEnabled = true
      v2SigningEnabled = true
    }
  }

  aaptOptions {
    {{android_aapt}}
    noCompress {{android_aapt_no_compress}}
  }

{{android_external_native_build}}

  productFlavors {
    dev {
      buildConfigField "String", "URL", "\"{{TODO}}\""
    }

    live {
      buildConfigField "String", "URL", "\"{{TODO}}\""
    }
  }

  buildTypes {
    release {
      minifyEnabled true
      proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
      //productFlavors.dev
      productFlavors.live
      {{android_buildtypes_release_config}}
    }

    debug {
      minifyEnabled false
      proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
      productFlavors.dev
      //productFlavors.live
    }
  }
}

dependencies {
  implementation "org.jetbrains.kotlin:kotlin-stdlib-jdk7:$kotlin_version"
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3'
  implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3'
  implementation 'androidx.fragment:fragment-ktx:1.7.1'
  implementation 'androidx.lifecycle:lifecycle-process:2.7.0'
  implementation 'androidx.appcompat:appcompat:1.6.1'
  implementation 'androidx.core:core-ktx:1.13.0'
  implementation 'androidx.webkit:webkit:1.9.0'
}
)GROOVY";

//
// Android top level `settings.gradle` in Groovy Syntax
//
constexpr auto gGradleSettings = R"GROOVY(
rootProject.name = "{{build_name}}"
startParameter.offline = false
include ':app'
startParameter.offline=false
)GROOVY";

//
// Android top level `gradle.properties`
//
constexpr auto gGradleProperties = R"GRADLE(
org.gradle.jvmargs=-Xmx2048m
org.gradle.parallel=true

android.useAndroidX=true
android.enableJetifier=true
android.suppressUnsupportedCompileSdk=34
android.experimental.legacyTransform.forceNonIncremental=true

kotlin.code.style=official
)GRADLE";

//
// Android cpp `Android.mk` file
//
constexpr auto gAndroidMakefile = R"MAKE(
LOCAL_PATH := $(call my-dir)

## libuv.a
include $(CLEAR_VARS)
LOCAL_MODULE := libuv

LOCAL_SRC_FILES = ../libs/$(TARGET_ARCH_ABI)/libuv.a
include $(PREBUILT_STATIC_LIBRARY)

## libllama.a
include $(CLEAR_VARS)
LOCAL_MODULE := libllama

LOCAL_SRC_FILES = ../libs/$(TARGET_ARCH_ABI)/libllama.a
include $(PREBUILT_STATIC_LIBRARY)

## libsocket-runtime.a
include $(CLEAR_VARS)
LOCAL_MODULE := libsocket-runtime-static

LOCAL_SRC_FILES = ../libs/$(TARGET_ARCH_ABI)/libsocket-runtime.a
include $(PREBUILT_STATIC_LIBRARY)

## Injected extensions
{{__android_native_extensions_context}}

## libsocket-runtime.so
include $(CLEAR_VARS)
LOCAL_MODULE := socket-runtime

LOCAL_CFLAGS +=                                                                \
  -std=c++2a                                                                   \
  -g                                                                           \
  -I$(LOCAL_PATH)/include                                                      \
  -I$(LOCAL_PATH)                                                              \
  -pthreads                                                                    \
  -fexceptions                                                                 \
  -fPIC                                                                        \
  -frtti                                                                       \
  -fsigned-char                                                                \
  -O0

LOCAL_CFLAGS += {{cflags}}

LOCAL_LDLIBS := -landroid -llog
LOCAL_SRC_FILES =                                                              \
  init.cc

LOCAL_STATIC_LIBRARIES :=                                                      \
  libllama                                                                     \

LOCAL_WHOLE_STATIC_LIBRARIES :=                                                \
  libuv                                                                        \
  libsocket-runtime-static

include $(BUILD_SHARED_LIBRARY)

## Custom userspace Android NDK
{{android_native_make_context}}
)MAKE";

//
// Android cpp `Application.mk` file
//
constexpr auto gAndroidApplicationMakefile = R"MAKE(
APP_ABI := {{android_native_abis}}
APP_STL := c++_static
)MAKE";

//
// Android `proguard-rules.pro` file
//
constexpr auto gProGuardRules = R"PGR(
# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile
)PGR";

//
// Android `layout/web_view.xml`
//
constexpr auto gAndroidLayoutWebView = R"XML(
<?xml version="1.0" encoding="utf-8"?>
<WebView
  xmlns:android="http://schemas.android.com/apk/res/android"
  xmlns:app="http://schemas.android.com/apk/res-auto"
  android:id="@+id/webview"
  android:layout_width="match_parent"
  android:layout_height="match_parent"
/>
)XML";

//
// Android `layout/window_container_view.xml`
//
constexpr auto gAndroidLayoutWindowContainerView = R"XML(
<?xml version="1.0" encoding="utf-8"?>
<FrameLayout
  xmlns:android="http://schemas.android.com/apk/res/android"
  android:layout_width="match_parent"
  android:layout_height="match_parent"
>
  <androidx.fragment.app.FragmentContainerView
    xmlns:android="http://schemas.android.com/apk/res/android"
    android:id="@+id/window"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
  />
</FrameLayout>
)XML";

constexpr auto gAndroidValuesStrings = R"XML(
<resources>
  <string name="app_name">{{meta_title}}</string>
</resources>
)XML";

//
// XCode Build Config
//
constexpr auto gXCodeScheme = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<Scheme
   LastUpgradeVersion = "1310"
   version = "1.3">
   <BuildAction
      parallelizeBuildables = "YES"
      buildImplicitDependencies = "YES">
      <BuildActionEntries>
         <BuildActionEntry
            buildForTesting = "YES"
            buildForRunning = "YES"
            buildForProfiling = "YES"
            buildForArchiving = "YES"
            buildForAnalyzing = "YES">
            <BuildableReference
               BuildableIdentifier = "primary"
               BlueprintIdentifier = "29124C4927613369001832A0"
               BuildableName = "{{build_name}}.app"
               BlueprintName = "{{build_name}}"
               ReferencedContainer = "container:{{build_name}}.xcodeproj">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <TestAction
      buildConfiguration = "Debug"
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      shouldUseLaunchSchemeArgsEnv = "YES">
      <Testables>
      </Testables>
   </TestAction>
   <LaunchAction
      buildConfiguration = "Debug"
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      launchStyle = "0"
      useCustomWorkingDirectory = "NO"
      ignoresPersistentStateOnLaunch = "NO"
      debugDocumentVersioning = "YES"
      debugServiceExtension = "internal"
      allowLocationSimulation = "YES">
      <BuildableProductRunnable
         runnableDebuggingMode = "0">
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "29124C4927613369001832A0"
            BuildableName = "{{build_name}}.app"
            BlueprintName = "{{build_name}}"
            ReferencedContainer = "container:{{build_name}}.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </LaunchAction>
   <ProfileAction
      buildConfiguration = "Release"
      shouldUseLaunchSchemeArgsEnv = "YES"
      savedToolIdentifier = ""
      useCustomWorkingDirectory = "NO"
      debugDocumentVersioning = "YES">
      <BuildableProductRunnable
         runnableDebuggingMode = "0">
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "29124C4927613369001832A0"
            BuildableName = "{{build_name}}.app"
            BlueprintName = "{{build_name}}"
            ReferencedContainer = "container:{{build_name}}.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </ProfileAction>
   <AnalyzeAction
      buildConfiguration = "Debug">
   </AnalyzeAction>
   <ArchiveAction
      buildConfiguration = "Release"
      customArchiveName = "{{build_name}}"
      revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>)XML";

constexpr auto gStoryboardLaunchScreen = R"XML(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<document type="com.apple.InterfaceBuilder3.CocoaTouch.Storyboard.XIB" version="3.0" toolsVersion="13122.16" targetRuntime="iOS.CocoaTouch" propertyAccessControl="none" useAutolayout="YES" launchScreen="YES" useTraitCollections="YES" useSafeAreas="YES" colorMatched="YES" initialViewController="01J-lp-oVM">
  <dependencies>
    <plugIn identifier="com.apple.InterfaceBuilder.IBCocoaTouchPlugin" version="13104.12"/>
    <capability name="Safe area layout guides" minToolsVersion="9.0"/>
    <capability name="documents saved in the Xcode 8 format" minToolsVersion="8.0"/>
  </dependencies>
  <scenes>
    <!--View Controller-->
    <scene sceneID="EHf-IW-A2E">
      <objects>
        <viewController id="01J-lp-oVM" sceneMemberID="viewController">
          <view key="view" contentMode="scaleToFill" id="Ze5-6b-2t3">
            <rect key="frame" x="0.0" y="0.0" width="375" height="667"/>
            <autoresizingMask key="autoresizingMask" widthSizable="YES" heightSizable="YES"/>
            <color key="backgroundColor" xcode11CocoaTouchSystemColor="systemBackgroundColor" cocoaTouchSystemColor="whiteColor"/>
            <viewLayoutGuide key="safeArea" id="6Tk-OE-BBY"/>
          </view>
        </viewController>
        <placeholder placeholderIdentifier="IBFirstResponder" id="iYj-Kq-Ea1" userLabel="First Responder" sceneMemberID="firstResponder"/>
      </objects>
      <point key="canvasLocation" x="53" y="375"/>
    </scene>
  </scenes>
</document>
)XML";

constexpr auto gDefaultConfig = R"INI(
;  ___  __   ___      __ ____
; /__  /  / /   /_/  /_   /
; __/ /__/ /__ /  \ /__  /
;
; Socket ⚡︎ Runtime · A modern runtime for Web Apps · v{{ssc_version}}
;
; Note that "~" alias won't expand to the home directory in any of the config
; files. Use the full path instead.


[build]

; ssc will copy everything in this directory to the build output directory.
; This is useful when you want to avoid bundling or want to use tools like
; vite, webpack, rollup, etc. to build your project and then copy output to
; the Socket bundle resources directory.
; default value: "src"
copy = "src"

; An ini file that maps files from the source directory to the build directory.
; copy_map = src/mapping.ini

; An list of environment variables, separated by commas.
env = USER, TMPDIR, PWD

; Advanced Compiler Settings (ie C++ compiler -02, -03, etc).
flags = -O3

; If true, the window will never be displayed.
; default value: false
headless = false

; The name of the program and executable to be output. Can't contain spaces or special characters. Required field.
name = "{{project_name}}"

; The binary output path. It's recommended to add this path to .gitignore.
; default value: "build"
output = "build"

; The build script. It runs before the `[build] copy` phase.
; script = "npm run build"


[build.script]

; If true, it will pass build arguments to the build script. WARNING: this could be deprecated in the future.
; default value: false
forward_arguments = false


[build.watch]

; Configure your project to watch for sources that could change when running `ssc`.
; Could be a string or an array of strings
sources[] = "src"


[webview]

; Make root open index.html
; default value: "/"
root = "/"

; Set default 'index.html' path to open for implicit routes
; default value: ""
; default_index  = ""

; Tell the webview to watch for changes in its resources
; default value: false
watch = true

; Custom headers injected on all webview routes
; default value: ""
; headers[] = "X-Custom-Header: Some-Value"

[webview.watch]
; Configure webview to reload when a file changes
; default value: true
reload = true

; Timeout in milliseconds to wait for service worker to reload before reloading webview
; default value: 500
service_worker_reload_timeout = 500

; Mount file system paths in webview navigator
[webview.navigator.mounts]

; $HOST_HOME/directory-in-home-folder/ = /mount/path/in/navigator
; $HOST_CONTAINER/directory-app-container/ = /mount/path/in/navigator
; $HOST_PROCESS_WORKING_DIRECTORY/directory-in-app-process-working-directory/ = /mount/path/in/navigator

; Specify allowed navigator navigation policy patterns
[webview.navigator.policies]
; allowed[] = "https://*.example.com/*"

[permissions]

; Allow/Disallow fullscreen in application
; default value: true
; allow_fullscreen = true

; Allow/Disallow microphone in application
; default value: true
; allow_microphone = true

; Allow/Disallow camera in application
; default value: true
; allow_camera = true

; Allow/Disallow user media (microphone + camera) in application
; default value: true
; allow_user_media = true

; Allow/Disallow geolocation in application
; default value: true
; allow_geolocation = true

; Allow/Disallow notifications in application
; default value: true
; allow_notifications = true

; Allow/Disallow sensors in application
; default value: true
; allow_sensors = true

; Allow/Disallow clipboard in application
; default value: true
; allow_clipboard = true

; Allow/Disallow bluetooth in application
; default value: true
; allow_bluetooth = true

; Allow/Disallow data access in application
; default value: true
; allow_data_access = true

; Allow/Disallow AirPlay access in application (macOS/iOS) only
; default value: true
; allow_airplay = true

; Allow/Disallow HotKey binding registration (desktop only)
; default value: true
; allow_hotkeys = true

[debug]

; Advanced Compiler Settings for debug purposes (ie C++ compiler -g, etc).
flags = "-g"


[meta]

; A unique ID that identifies the bundle (used by all app stores).
; It's required when `[meta] type` is not `"extension"`.
; It should be in a reverse DNS notation https://developer.apple.com/documentation/bundleresources/information_property_list/cfbundleidentifier#discussion
bundle_identifier = "com.{{project_name}}"

; A unique application protocol scheme to support deep linking
; If this value is not defined, then it is derived from the `[meta] bundle_identifier` value
application_protocol = "{{project_name}}"

; A string that gets used in the about dialog and package meta info.
; copyright = "(c) Beep Boop Corp. 1985"

; A short description of the app.
; description = "A UI for the beep boop network"

; Set the limit of files that can be opened by your process.
file_limit = 1024

; Localization
lang = "en-us"

; A String used in the about dialog and meta info.
; maintainer = "Beep Boop Corp."

; The title of the app used in metadata files. This is NOT a window title. Can contain spaces and special characters. Defaults to name in a [build] section.
title = "{{project_name}}"

; Builds an extension when set to "extension".
; default value: ""
; type = ""

; A string that indicates the version of the application. It should be a semver triple like 1.2.3. Defaults to 1.0.0.
version = 1.0.0


[android]

; Extensions of files that will not be stored compressed in the APK.
aapt_no_compress = ""

; Enables gradle based ndk build rather than using external native build (standard ndk is the old slow way)
enable_standard_ndk_build = false

; Name of the MainActivity class. Could be overwritten by custom native code.
main_activity = ""

; Which permissions does your application need: https://developer.android.com/guide/topics/permissions/overview
manifest_permissions = ""

; To restrict the set of ABIs that your application supports, set them here.
native_abis = ""

; Used for adding custom source files and related compiler attributes.
native_cflags = ""
native_sources = ""
native_makefile = ""
sources = ""

; The icon to use for identifying your app on Android.
icon = "src/icon.png"

; The various sizes and scales of the icons to create, required minimum are listed by default.
icon_sizes = "512@1x"


[ios]

; signing guide: https://socketsupply.co/guides/#ios-1
codesign_identity = ""

; Describes how Xcode should export the archive. Available options: app-store, package, release-testing, enterprise, development, and developer-id.
distribution_method = "release-testing"

; A path to the provisioning profile used for signing iOS app.
provisioning_profile = ""

; which device to target when building for the simulator.
simulator_device = "iPhone 14"

; Indicate to Apple if you are using encryption that is not exempt.
; default value: false
; nonexempt_encryption = false

; The icon to use for identifying your app on iOS.
icon = "src/icon.png"

; The various sizes and scales of the icons to create, required minimum are listed by default.
icon_sizes = "29@1x 29@2x 29@3x 40@2x 40@3x 57@1x 57@2x 60@2x 60@3x"

[linux]

; Helps to make your app searchable in Linux desktop environments.
categories = "Developer Tools"

; The command to execute to spawn the "back-end" process.
; cmd = "node backend/index.js"

; The icon to use for identifying your app in Linux desktop environments.
icon = "src/icon.png"

; The various sizes and scales of the icons to create, required minimum are listed by default.
icon_sizes = "512@1x"


[mac]

; A category in the App Store
category = ""

; The command to execute to spawn the "back-end" process.
; cmd = "node backend/index.js"

; TODO Signing guide: https://socketsupply.co/guides/#code-signing-certificates
codesign_identity = ""

; Additional paths to codesign
codesign_paths = ""

; Minimum supported MacOS version
; default value: "13.0.0"
; minimum_supported_version = "13.0.0"

; If titlebar_style is "hiddenInset", this will determine the x and y offsets of the window controls (traffic lights).
; window_control_offsets = "10x24"

; The icon to use for identifying your app on MacOS.
icon = "src/icon.png"

; The various sizes and scales of the icons to create, required minimum are listed by default.
icon_sizes = "16@1x 32@1x 128@1x"


[native]

; Files that should be added to the compile step.
files = native-module1.cc native-module2.cc

; Extra Headers
headers = native-module1.hh


[win]

; The command to execute to spawn the “back-end” process.
; cmd = "node backend/index.js"

; The icon to use for identifying your app on Windows, relative to copied path resources
logo = "icon.ico"

; A relative path to the pfx file used for signing.
; pfx = "certs/cert.pfx"

; The signing information needed by the appx api.
; publisher = "CN=Beep Boop Corp., O=Beep Boop Corp., L=San Francisco, S=California, C=US"

; The icon to use for identifying your app on Windows.
icon = "src/icon.ico"

; The various sizes and scales of the icons to create, required minimum are listed by default.
icon_sizes = "512@1x"


[window]

; The initial height of the first window in pixels or as a percentage of the screen.
height = 50%

; The initial width of the first window in pixels or as a percentage of the screen.
width = 50%

; The initial color of the window in dark mode. If not provided, matches the current theme.
; default value: ""
; backgroundColorDark = "rgba(0, 0, 0, 1)"

; The initial color of the window in light mode. If not provided, matches the current theme.
; default value: ""
; backgroundColorLight = "rgba(255, 255, 255, 1)"

; Determine if the titlebar style (hidden, hiddenInset)
; default value: ""
; titlebar_style = "hiddenInset"

; Maximum height of the window in pixels or as a percentage of the screen.
; default value: 100%
; max_height = 100%

; Maximum width of the window in pixels or as a percentage of the screen.
; default value: 100%
; max_width = 100%

; Minimum height of the window in pixels or as a percentage of the screen.
; default value: 0
; min_height = 0

; Minimum width of the window in pixels or as a percentage of the screen.
; default value: 0
; min_width = 0

; Determines if the window has a title bar and border.
; default value: false
; frameless = false

; Determines if the window is resizable.
; default value: true
; resizable = true

; Determines if the window is maximizable.
; default value: true
; maximizable = true

; Determines if the window is minimizable.
; default value: true
; minimizable = true

; Determines if the window is closable.
; default value: true
; closable = true

; Determines the window is utility window.
; default value: false
; utility = false

[window.alert]

; The title that appears in the 'alert', 'prompt', and 'confirm' dialogs. If this value is not present, then the application title is used instead. Currently only supported on iOS/macOS.
; defalut value = ""
; title = ""


[application]

; If agent is set to true, the app will not display in the tab/window switcher or dock/task-bar etc. Useful if you are building a tray-only app.
; default value: false
; agent = true


[tray]

; The icon to be displayed in the operating system tray. On Windows, you may need to use ICO format.
; defalut value = ""
; icon = "src/icon.png"


[headless]

; The headless runner command. It is used when no OS specific runner is set.
runner = ""
; The headless runner command flags. It is used when no OS specific runner is set.
runner_flags = ""
; The headless runner command for Android
runner_android = ""
; The headless runner command flags for Android
runner_android_flags = ""
; The headless runner command for iOS
runner_ios = ""
; The headless runner command flags for iOS
runner_ios_flags = ""
; The headless runner command for Linux
runner_linux = ""
; The headless runner command flags for Linux
runner_linux_flags = ""
; The headless runner command for MacOS
runner_mac = ""
; The headless runner command flags for MacOS
runner_mac_flags = ""
; The headless runner command for Windows
runner_win32 = ""
; The headless runner command flags for Windows
runner_win32_flags = ""

)INI";

constexpr auto gDefaultGitignore = R"GITIGNORE(
logs
# Logs
*.log
*.dat
npm-debug.log*
yarn-debug.log*
yarn-error.log*
lerna-debug.log*
.pnpm-debug.log*
.sscrc
.*.env

# Ignore all files in the node_modules folder
node_modules/

# Default output directory
build/

# Provisioning profile
*.mobileprovision

# extension build artifacts
*.o

)GITIGNORE";
