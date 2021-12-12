# Opkit for iOS

## Prerequisites

### Software

While XCode is not required for Opkit Desktop Apps, you will *want* to download
and install Xcode because it provides a device simulator, also Opkit will use
it. Opkit makes it easy to transition to a regular Xcode project.

### Administravia

Only signed software will run on iOS. "Provisioning profiles" contain keys that
link a device to a developer account, allowing you to test your apps before
distributing them to `Test Flight` or the Apple App Store. There are some
administrative steps required before you will be able to build an iOS app.

- Sign up for a (free) [Apple Developer][https://developer.apple.com/] account.
- Create an [App Store Connect][000] record for your app.
- Register your device(s) for testing [here][00].
- Create an `iOS Distribution (App Store and Ad Hoc)` certificate [here][01].
Download the certificate and double click it to install it to the Apple
`Keychain`.
- Create a `New Provisioning Profile` [here][02]. Then download it and add it
to your project.

```settings
apple_teamId: Z3M838H537
apple_distribution_method: ad-hoc
apple_signing_certificate: iOS Distribution (App Store and Ad Hoc)
apple_provisioning_profile: src/foobar.mobileprovision
```

```bash
opkit . -r # start the simulator
```

```bash
opkit . -ios -c -p -xd # package for distribution
```

# TECHNICAL

```bash
/usr/libexec/plistbuddy -c Print:UUID \
  /dev/stdin <<< `security cms -D -i build/opkit.xcarchive/Products/Applications/opkit.app/embedded.mobileprovision`
```

Rename the file with the uuid and "install" it by copying it into the
well-known location for provisioning profiles.

```bash
cp {{uuid}}.mobileprovision $HOME/Library/MobileDevice/Provisioning\ Profiles/`
```

For later distirbution, you will want to create a file called
`exportOptions.plist` and add the `uuid` extracted from the provisioning
profile, as well as the `signingCertificate` (that's the name of the
certificate, ie `iPhone Distribution: Operator Tools Inc. (Z3M838H537)`).

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>method</key>
    <string>ad-hoc</string>
    <key>teamID</key>
    <string>{{teamId}}</string>
    <key>uploadBitcode</key>
    <true/>
    <key>compileBitcode</key>
    <true/>
    <key>uploadSymbols</key>
    <true/>
    <key>signingStyle</key>
    <string>manual</string>
    <key>signingCertificate</key>
    <string>{{signinigCertificateId}}</string>
    <key>provisioningProfiles</key>
    <dict>
      <key>{{bundle-id}}</key>
      <string>{{uuid}}</string>
    </dict>
  </dict>
</plist>
```

## Distribution

### Create and Export an "Archive" (.ipa file)

```bash
xcodebuild -quiet -scheme {{name}} clean archive \
  -archivePath build/{{name}} \
  -destination 'generic/platform=iOS'
```

```bash
xcodebuild -quiet -exportArchive \
  -archivePath build/{{name}}.xcarchive \
  -exportPath build/{{name}}.ipa \
  -exportOptionsPlist exportOptions.plist
```

To this this distirbution on your iPhone, open the the `Apple Configurator 2`
app and drag the inner `{{name}}.ipa` file onto your phone.

### Upload to App Store

```bash
xcrun altool --validate-app \
  -f file \
  -t platform \
  -u username [-p password] \
  [--output-format xml]
```

```bash
xcrun altool --upload-app \
  -f file \
  -t platform \
  -u username [-p password] \
  [—output-format xml]
```

## Development

### Running the iOS Simulator
To create a iOS VM.

```bash
xcrun simctl \
  create <YourSimulatorDeviceName> \
  com.apple.CoreSimulator.SimDeviceType.iPhone-13-Pro \
  com.apple.CoreSimulator.SimRuntime.iOS-15-0 > boot-id.txt
```

To boot the VM that was created from the command above.

```bash
xcrun simctl boot `cat boot-id.txt`
```

To run the simulator.

```bash
open /Applications/Xcode.app/Contents/Developer/Applications/Simulator.app/
```

To install your app into the current VM of the running simulator.

```bash
xcrun simctl install booted test/ios/dist/TestExample-dev.app
```

## Debugging

You can run [`lldb`][1] and attach to a process, for example...

```bash
process attach --name TestExample-dev
```

### Logging

To see logs, open `Console.app` (installed on MacOS by default) and in the
right side panel pick <YourSimulatorDeviceName>. You can filter by `OPKIT`
to see the logs that your app outputs.

###  Loading your app onto a real phone.

[000]:https://appstoreconnect.apple.com/apps
[00]:https://developer.apple.com/account/resources/devices/add
[01]:https://developer.apple.com/account/resources/certificates/add
[02]:https://developer.apple.com/account/resources/profiles/add
[1]:https://developer.apple.com/library/archive/documentation/IDEs/Conceptual/gdb_to_lldb_transition_guide/document/lldb-terminal-workflow-tutorial.html
