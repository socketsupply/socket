#include <mutex>
#include <thread>

//
// Darwin config
//
constexpr auto gPListInfo = R"XML(<?xml version="1.0" encoding="UTF-8"?>
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

constexpr auto gPListInfoMAS = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>UIRequiredDeviceCapabilities</key>
	<dict/>
	<key>artistId</key>
	<integer>{{appleId}}</integer>
	<key>artistName</key>
	<string>{{maintainer}}</string>
	<key>asset-info</key>
	<dict>
		<key>file-size</key>
		<integer>1903386</integer>
		<key>flavor</key>
		<string>10:purple</string>
	</dict>
	<key>bundleDisplayName</key>
	<string>{{name}}</string>
	<key>bundleShortVersionString</key>
	<string>{{versionShort}}</string>
	<key>bundleVersion</key>
	<string>24</string>
	<key>copyright</key>
	<string>{{copyRight}</string>
	<key>drmVersionNumber</key>
	<integer>0</integer>
	<key>fileExtension</key>
	<string>.app</string>
	<key>gameCenterEnabled</key>
	<false/>
	<key>gameCenterEverEnabled</key>
	<false/>
	<key>genre</key>
	<string>Utilities</string>
	<key>genreId</key>
	<integer>6002</integer>
	<key>itemId</key>
	<integer>1599549293</integer>
	<key>itemName</key>
	<string>{{name}}.</string>
	<key>kind</key>
	<string>software</string>
	<key>playlistName</key>
	<string>{{maintainer}}</string>
	<key>product-type</key>
	<string>ios-app</string>
	<key>purchaseDate</key>
	<string></string>
	<key>rating</key>
	<dict>
		<key>content</key>
		<string></string>
		<key>label</key>
		<string>4+</string>
		<key>rank</key>
		<integer>100</integer>
		<key>system</key>
		<string>itunes-games</string>
	</dict>
	<key>releaseDate</key>
	<string>2021-01-01T03:54:02Z</string>
	<key>requiresRosetta</key>
	<false/>
	<key>runsOnAppleSilicon</key>
	<true/>
	<key>runsOnIntel</key>
	<false/>
	<key>s</key>
	<integer>143452</integer>
	<key>software-platform</key>
	<string>ios</string>
	<key>softwareIconNeedsShine</key>
	<false/>
	<key>softwareSupportedDeviceIds</key>
	<array>
		<integer>2</integer>
		<integer>9</integer>
		<integer>4</integer>
		<integer>13</integer>
	</array>
	<key>softwareVersionBundleId</key>
	<string>{{bundle_identifier}}</string>
	<key>softwareVersionExternalIdentifier</key>
	<integer>833237506</integer>
	<key>softwareVersionExternalIdentifiers</key>
	<array>
		<integer>717753355</integer>
	</array>
	<key>vendorId</key>
	<integer>26798800</integer>
	<key>versionRestrictions</key>
	<integer>0</integer>
</dict>
</plist>)XML";

constexpr auto gPListInfoPbxproj = R"XML(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 55;
	objects = {

/* Begin PBXNativeTarget section */
		29124C4927613369001832A0 /* opkit */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget "opkit" */;
			buildPhases = (
				29124C4627613369001832A0 /* Sources */,
				29124C4727613369001832A0 /* Frameworks */,
				29124C4827613369001832A0 /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = opkit;
			productName = opkit;
			productReference = 29124C4A27613369001832A0 /* opkit.app */;
			productType = "com.apple.product-type.application";
		};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		29124C4227613369001832A0 /* Project object */ = {
			isa = PBXProject;
			attributes = {
				BuildIndependentTargetsInParallel = 1;
				LastUpgradeCheck = 1310;
				TargetAttributes = {
					29124C4927613369001832A0 = {
						CreatedOnToolsVersion = 13.1;
					};
					29124C642761336B001832A0 = {
						CreatedOnToolsVersion = 13.1;
						TestTargetID = 29124C4927613369001832A0;
					};
					29124C6E2761336B001832A0 = {
						CreatedOnToolsVersion = 13.1;
						TestTargetID = 29124C4927613369001832A0;
					};
				};
			};
			buildConfigurationList = 29124C4527613369001832A0 /* Build configuration list for PBXProject "opkit" */;
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
				29124C4927613369001832A0 /* opkit */,
			);
		};
/* End PBXProject section */

/* Begin XCBuildConfiguration section */
		29124C772761336B001832A0 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
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
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
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
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 15.0;
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				SDKROOT = iphoneos;
			};
			name = Debug;
		};
		29124C782761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++17";
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
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
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
				IPHONEOS_DEPLOYMENT_TARGET = 15.0;
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				SDKROOT = iphoneos;
				VALIDATE_PRODUCT = YES;
			};
			name = Release;
		};
		29124C7A2761336B001832A0 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_IDENTITY = "Apple Development";
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = Z3M838H537;
				GENERATE_INFOPLIST_FILE = YES;
				INFOPLIST_FILE = opkit/Info.plist;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchStoryboardName = LaunchScreen;
				INFOPLIST_KEY_UIMainStoryboardFile = Main;
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = opkit;
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "";
				SWIFT_EMIT_LOC_STRINGS = YES;
				TARGETED_DEVICE_FAMILY = "1,2";
			};
			name = Debug;
		};
		29124C7B2761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_IDENTITY = "Apple Development";
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = Z3M838H537;
				GENERATE_INFOPLIST_FILE = YES;
				INFOPLIST_FILE = opkit/Info.plist;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchStoryboardName = LaunchScreen;
				INFOPLIST_KEY_UIMainStoryboardFile = Main;
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = opkit;
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "";
				SWIFT_EMIT_LOC_STRINGS = YES;
				TARGETED_DEVICE_FAMILY = "1,2";
			};
			name = Release;
		};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		29124C4527613369001832A0 /* Build configuration list for PBXProject "opkit" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				29124C772761336B001832A0 /* Debug */,
				29124C782761336B001832A0 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget "opkit" */ = {
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
}
)XML";

constexpr auto gPListInfoIOS = R"XML(<?xml version="1.0" encoding="UTF-8"?>
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
	<key>CFBundleShortVersionString</key>
	<string>{{versionShort}}</string>
	<key>CFBundleSignature</key>
	<string>OPERATOR</string>
	<key>CFBundleVersion</key>
	<string>{{version}}</string>
	<key>LSRequiresIPhoneOS</key>
	<true/>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>iPhoneOS</string>
	</array>
	<key>DTCompiler</key>
	<string>com.apple.compilers.llvm.clang.1_0</string>
	<key>DTPlatformBuild</key>
	<string>19A339</string>
	<key>DTPlatformName</key>
	<string>iphoneos</string>
	<key>DTPlatformVersion</key>
	<string>15.0</string>
  <key>UIDeviceFamily</key>
  <array>
    <integer>1</integer>
  </array>
	<key>DTSDKBuild</key>
	<string>19A339</string>
	<key>DTSDKName</key>
	<string>iphoneos15.0</string>
	<key>DTXcode</key>
	<string>1310</string>
	<key>DTXcodeBuild</key>
	<string>13A1030d</string>
  <key>MinimumOSVersion</key>
	<string>15.0</string>
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
</plist>)XML";

constexpr auto gBuildSchemaIOS = R"XML(<?xml version="1.0" encoding="UTF-8"?>
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
               BuildableName = "opkit.app"
               BlueprintName = "opkit"
               ReferencedContainer = "container:opkit.xcodeproj">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
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
            BuildableName = "opkit.app"
            BlueprintName = "opkit"
            ReferencedContainer = "container:opkit.xcodeproj">
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
            BuildableName = "opkit.app"
            BlueprintName = "opkit"
            ReferencedContainer = "container:opkit.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </ProfileAction>
   <AnalyzeAction
      buildConfiguration = "Debug">
   </AnalyzeAction>
   <ArchiveAction
      buildConfiguration = "Release"
      revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>)XML";

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
