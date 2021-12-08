#include <mutex>
#include <thread>

//
// Darwin config
//
constexpr auto gPListInfo = R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>
  <string>{{name}}</string>
  <key>DTSDKName</key>
  <string>macosx10.13</string>
  <key>DTXcode</key>
  <string>0941</string>
  <key>NSHumanReadableCopyright</key>
  <string>{{copyRight}}</string>
  <key>DTSDKBuild</key>
  <string>10.13</string>
  <key>CFBundleVersion</key>
  <string>{{version}}</string>
  <key>BuildMachineOSBuild</key>
  <string>17D102</string>
  <key>NSCameraUsageDescription</key>
  <string>This app needs access to the camera</string>
  <key>NSMainNibFile</key>
  <string>MainMenu</string>
  <key>LSMultipleInstancesProhibited</key>
  <true/>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleIconFile</key>
  <string>index.icns</string>
  <key>CFBundleShortVersionString</key>
  <string>{{versionShort} Build {{revision}}</string>
  <key>NSHighResolutionCapable</key>
  <true/>
  <key>NSMicrophoneUsageDescription</key>
  <string>This app needs access to the microphone</string>
  <key>NSAppTransportSecurity</key>
  <dict>
    <key>NSExceptionDomains</key>
    <dict>
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
    <key>NSAllowsArbitraryLoads</key>
    <false/>
    <key>NSAllowsLocalNetworking</key>
    <true/>
  </dict>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleExecutable</key>
  <string>{{executable}}</string>
  <key>DTCompiler</key>
  <string>com.apple.compilers.llvm.clang.1_0</string>
  <key>NSPrincipalClass</key>
  <string>AtomApplication</string>
  <key>NSRequiresAquaSystemAppearance</key>
  <false/>
  <key>CFBundleIdentifier</key>
  <string>{{bundle_identifier}}</string>
  <key>LSApplicationCategoryType</key>
  <string>{{mac_category}}</string>
  <key>DTXcodeBuild</key>
  <string>9F2000</string>
  <key>LSMinimumSystemVersion</key>
  <string>10.10.0</string>
  <key>CFBundleDisplayName</key>
  <string>{{name}}</string>
  <key>NSSupportsAutomaticGraphicsSwitching</key>
  <true/>
  <key>WKAppBoundDomains</key>
  <array>
      <string>localhost</string>
  </array>
  </dict>
</plist>
)XML";

constexpr auto gPListInfoMAS = R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>apple-id</key>
	<string>admin@operator.tc</string>
	<key>artistName</key>
	<string>Operator Tools Inc.</string>
	<key>bundleShortVersionString</key>
	<string>{{versionShort}</string>
	<key>bundleVersion</key>
	<string>{{version}}</string>
  <key>copyright</key>
  <string>{{copyright}}</string>
  <key>drmVersionNumber</key>
  <integer>0</integer>
  <key>fileExtension</key>
  <string>.app</string>
  <key>gameCenterEnabled</key>
  <false/>
  <key>gameCenterEverEnabled</key>
  <false/>
  <key>genre</key>
  <string>Games</string>
  <key>genreId</key>
  <integer>6014</integer>
  <key>itemName</key>
  <string>App Name</string>
  <key>kind</key>
  <string>software</string>
  <key>playlistArtistName</key>
  <string>Company, Inc.</string>
  <key>playlistName</key>
  <string>App Name</string>
  <key>releaseDate</key>
  <string>2015-11-18T03:23:10Z</string>
  <key>s</key>
  <integer>143441</integer>
  <key>softwareIconNeedsShine</key>
  <false/>
  <key>softwareSupportedDeviceIds</key>
  <array>
    <integer>9</integer>
  </array>
  <key>softwareVersionBundleId</key>
  <string>com.company.appid</string>
  <key>subgenres</key>
  <array>
    <dict>
      <key>gene</key>
      <string>Puzzle</string>
      <key>genreId</key>
      <integer>7012</integer>
    </dict>
    <dict>
      <key>genre</key>
      <string>Word</string>
      <key>genreId</key>
      <integer>7019</integer>
    </dict>
  </array>
  <key>versionRestrictions</key>
  <integer>16843008</integer>
  </dict>
</plist>
)XML";

constexpr auto gPListInfoIOS = R"XML(
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleDisplayName</key>
	<string>{{name}}</string>
	<key>CFBundleExecutable</key>
	<string>{{name}}</string>
	<key>CFBundleIdentifier</key>
	<string>{{bundle_identifier}}</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>{{name}}</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>{{versionShort}}</string>
	<key>CFBundleSignature</key>
	<string>OPERATOR</string>
	<key>CFBundleVersion</key>
	<string>{{version}}</string>
	<key>LSRequiresIPhoneOS</key>
	<true/>
  <key>UILaunchStoryboardName</key>
  <string>LaunchScreen</string>
  <key>CFBundleIconName</key>
  <string>AppIcon</string>
  <key>CFBundleIconFile</key>
  <string>index.png</string>
  <key>UIPrerenderedIcon</key>
  <true/>
	<key>UISupportedInterfaceOrientations</key>
	<array>
		<string>UIInterfaceOrientationPortrait</string>
		<string>UIInterfaceOrientationPortraitUpsideDown</string>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>
</dict>
</plist>
)XML";

//
// Linux config
//
constexpr auto gDestkopManifest = R"INI(
[Desktop Entry]
Encoding=UTF-8
Version={{version}}
Name={{name}}
Terminal=false
Type=Application
Exec={{linux_executable_path}}
Icon={{linux_icon_path}}
# Icon must be generated & appended by `toolbox`.
# Maybe set StartupWMClass ??
# StartupWMClass={{name}}
Comment={{description}}
Categories={{linux_categories}};
)INI";

constexpr auto gDebianManifest = R"DEB(Package: {{name}}
Version: {{versionShort}}
Architecture: {{arch}}
Maintainer: {{maintainer}}
Description: {{title}}
 {{description}}
)DEB";

//
// Windows config
//
constexpr auto gWindowsAppManifest = R"XML(<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/2010/manifest">
  <Identity Name="{bundle_identifier}"
    ProcessorArchitecture="neutral"
    Publisher="{{win_publisher}}"
    Version="{{versionShort}}.{{revision}}"
    ResourceId="{{executable}}"
  />

  <Properties>
    <DisplayName>{{title}}</DisplayName>
    <Description>{{description}}</Description>
    <Logo>{{win_logo}}</Logo>
    <PublisherDisplayName>{{maintainer}}</PublisherDisplayName>
  </Properties>

  <Resources>
    <Resource Language="{{lang}}"/>
  </Resources>

  <Applications>
    <Application Id="HelloWorld" StartPage="index.html">
      <VisualElements DisplayName="{{title}}"
        Logo="{{win_logo}}"
        SmallLogo="{{win_logo}}"
        Description="{{description}}"
        BackgroundColor="#AA00AA"
        ForegroundText="light"
      >
        <DefaultTile WideLogo="{{win_logo}}" />
        <SplashScreen Image="{{win_logo}}" />
      </VisualElements>
    </Application>

  </Applications>

  <Prerequisites>
    <OSMinVersion>6.3</OSMinVersion>
    <OSMaxVersionTested>10.0.19041</OSMaxVersionTested>
  </Prerequisites>
</Package>
)XML";
