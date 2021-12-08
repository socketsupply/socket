# iOS Example

## Project Contents

| File | Description | Required |
| :--- | :--- | ---: |
| settings.config | Describes properties used to build. | True |
| build.js | A script to copy files into the build. | False |
| ./src/Default-568h@2x.png | [Launch Image][0]. | True |
| ./src/.. | Arbitrary source files | False |

## Prerequisites

Xcode must be installed!

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

### Logging
To see logs, open `Console.app` (installed on MacOS by default) and in the right
side panel pick <YourSimulatorDeviceName>. You can filter by `OPKIT` to see the
logs that your app outputs.

Loading your app onto a real phone.

[0]:https://stackoverflow.com/questions/21668497/uiscreen-mainscreen-bounds-returning-wrong-size
