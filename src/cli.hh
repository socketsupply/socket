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
  <string>{{copyright}}</string>
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
  <string>{{version_short} Build {{revision}}</string>
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
Version: {{version_short}}
Architecture: {{arch}}
Maintainer: {{maintainer}}
Description: {{title}}
 {{description}}
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
  <Identity Name="{bundle_identifier}"
    ProcessorArchitecture="neutral"
    Publisher="{{win_publisher}}"
    Version="{{win_version}}"
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
      Id="HelloWorld"
      EntryPoint="Windows.FullTrustApplication"
      Executable="{{exe}}"
    >
      <uap:VisualElements DisplayName="{{title}}"
        Square150x150Logo="{{win_logo}}"
        Square44x44Logo="{{win_logo}}"
        Description="{{description}}"
        BackgroundColor="#20123A"
      >
        <uap:DefaultTile Wide310x150Logo="{{win_logo}}" />
      </uap:VisualElements>
      <Extensions>
        <uap3:Extension
          Category="windows.appExecutionAlias"
          EntryPoint="Windows.FullTrustApplication"
          Executable="{{exe}}"
        >
          <uap3:AppExecutionAlias>
            <desktop:ExecutionAlias Alias="{{exe}}" />
          </uap3:AppExecutionAlias>
        </uap3:Extension>
        <desktop2:Extension Category="windows.firewallRules">
          <desktop2:FirewallRules Executable="{{exe}}">
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
          </desktop2:FirewallRules>
        </desktop2:Extension>
      </Extensions>
    </Application>

  </Applications>
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
		290F7EBF2768C49000486988 /* UIKit.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 294A3C792763E9C6007B5B9A /* UIKit.framework */; };
		290F7F87276BC2B000486988 /* lib in Resources */ = {isa = PBXBuildFile; fileRef = 290F7F86276BC2B000486988 /* lib */; };
		290F7F88276BC2EE00486988 /* libuv.a in Frameworks */ = {isa = PBXBuildFile; fileRef = 290F7F82276BBE9C00486988 /* libuv.a */; };
		29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */ = {isa = PBXBuildFile; fileRef = 29124C5B2761336B001832A0 /* LaunchScreen.storyboard */; };
		294A3C502763E6BC007B5B9A /* ios.mm in Sources */ = {isa = PBXBuildFile; fileRef = 294A3C4E2763E5EB007B5B9A /* ios.mm */; };
		294A3C852764EAB7007B5B9A /* ui in Resources */ = {isa = PBXBuildFile; fileRef = 294A3C842764EAB7007B5B9A /* ui */; };
		294A3CA02768C429007B5B9A /* WebKit.framework in Frameworks */ = {isa = PBXBuildFile; fileRef = 294A3C7B2763EA7F007B5B9A /* WebKit.framework */; };
/* End PBXBuildFile section */

/* Begin PBXFileReference section */
		290F7F82276BBE9C00486988 /* libuv.a */ = {isa = PBXFileReference; lastKnownFileType = archive.ar; name = libuv.a; path = lib/uv/libuv.a; sourceTree = "<group>"; };
		290F7F86276BC2B000486988 /* lib */ = {isa = PBXFileReference; lastKnownFileType = folder; path = lib; sourceTree = "<group>"; };
		29124C4A27613369001832A0 /* op.app */ = {isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = op.app; sourceTree = BUILT_PRODUCTS_DIR; };
		29124C5C2761336B001832A0 /* Base */ = {isa = PBXFileReference; lastKnownFileType = file.storyboard; name = Base; path = Base.lproj/LaunchScreen.storyboard; sourceTree = "<group>"; };
		29124C5E2761336B001832A0 /* Info.plist */ = {isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; };
		294A3C4E2763E5EB007B5B9A /* ios.mm */ = {isa = PBXFileReference; explicitFileType = sourcecode.cpp.objcpp; indentWidth = 2; path = ios.mm; sourceTree = "<group>"; tabWidth = 2; };
		294A3C762763E930007B5B9A /* ios.hh */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.cpp.h; path = ios.hh; sourceTree = "<group>"; };
		294A3C792763E9C6007B5B9A /* UIKit.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = UIKit.framework; path = System/Library/Frameworks/UIKit.framework; sourceTree = SDKROOT; };
		294A3C7B2763EA7F007B5B9A /* WebKit.framework */ = {isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = WebKit.framework; path = System/Library/Frameworks/WebKit.framework; sourceTree = SDKROOT; };
		294A3C7F27649D27007B5B9A /* preload.hh */ = {isa = PBXFileReference; fileEncoding = 4; indentWidth = 2; lastKnownFileType = sourcecode.cpp.h; path = preload.hh; sourceTree = "<group>"; tabWidth = 2; };
		294A3C8027649DD9007B5B9A /* common.hh */ = {isa = PBXFileReference; fileEncoding = 4; indentWidth = 2; lastKnownFileType = sourcecode.cpp.h; path = common.hh; sourceTree = "<group>"; tabWidth = 2; };
		294A3C842764EAB7007B5B9A /* ui */ = {isa = PBXFileReference; lastKnownFileType = folder; path = ui; sourceTree = "<group>"; };
		294A3C9027677424007B5B9A /* op.entitlements */ = {isa = PBXFileReference; lastKnownFileType = text.plist.entitlements; path = op.entitlements; sourceTree = "<group>"; };
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		29124C4727613369001832A0 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
				290F7F88276BC2EE00486988 /* libuv.a in Frameworks */,
				294A3CA02768C429007B5B9A /* WebKit.framework in Frameworks */,
				290F7EBF2768C49000486988 /* UIKit.framework in Frameworks */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		290F7F89276CBF9900486988 /* Recovered References */ = {
			isa = PBXGroup;
			children = (
				290F7F82276BBE9C00486988 /* libuv.a */,
			);
			name = "Recovered References";
			sourceTree = "<group>";
		};
		29124C4127613369001832A0 = {
			isa = PBXGroup;
			children = (
				290F7F86276BC2B000486988 /* lib */,
				294A3C9027677424007B5B9A /* op.entitlements */,
				294A3C842764EAB7007B5B9A /* ui */,
				294A3C8027649DD9007B5B9A /* common.hh */,
				294A3C7F27649D27007B5B9A /* preload.hh */,
				29124C5B2761336B001832A0 /* LaunchScreen.storyboard */,
				29124C5E2761336B001832A0 /* Info.plist */,
				294A3C762763E930007B5B9A /* ios.hh */,
				294A3C4E2763E5EB007B5B9A /* ios.mm */,
				29124C4B27613369001832A0 /* Products */,
				294A3C782763E9C6007B5B9A /* Frameworks */,
				290F7F89276CBF9900486988 /* Recovered References */,
			);
			sourceTree = "<group>";
		};
		29124C4B27613369001832A0 /* Products */ = {
			isa = PBXGroup;
			children = (
				29124C4A27613369001832A0 /* op.app */,
			);
			name = Products;
			sourceTree = "<group>";
		};
		294A3C782763E9C6007B5B9A /* Frameworks */ = {
			isa = PBXGroup;
			children = (
				294A3C7B2763EA7F007B5B9A /* WebKit.framework */,
				294A3C792763E9C6007B5B9A /* UIKit.framework */,
			);
			name = Frameworks;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
		29124C4927613369001832A0 /* op */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget "op" */;
			buildPhases = (
				29124C4627613369001832A0 /* Sources */,
				29124C4727613369001832A0 /* Frameworks */,
				29124C4827613369001832A0 /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = op;
			productName = op;
			productReference = 29124C4A27613369001832A0 /* op.app */;
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
				};
			};
			buildConfigurationList = 29124C4527613369001832A0 /* Build configuration list for PBXProject "op" */;
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
				29124C4927613369001832A0 /* op */,
			);
		};
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
		29124C4827613369001832A0 /* Resources */ = {
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				290F7F87276BC2B000486988 /* lib in Resources */,
				29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */,
				294A3C852764EAB7007B5B9A /* ui in Resources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
		29124C4627613369001832A0 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				294A3C502763E6BC007B5B9A /* ios.mm in Sources */,
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
				CLANG_ANALYZER_NONNULL = YES;
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
				SDKROOT = iphoneos;
				SUPPORTED_PLATFORMS = "iphonesimulator iphoneos";
			};
			name = Debug;
		};
		29124C782761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
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
				SUPPORTED_PLATFORMS = "iphonesimulator iphoneos";
				VALIDATE_PRODUCT = YES;
			};
			name = Release;
		};
		29124C7A2761336B001832A0 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ARCHS = arm64;
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_ENTITLEMENTS = op.entitlements;
				CODE_SIGN_IDENTITY = "iPhone Distribution";
				CODE_SIGN_STYLE = Manual;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = Z3M838H537;
				ENABLE_BITCODE = YES;
				"EXCLUDED_ARCHS[sdk=*]" = "armv7 armv7s i386 arm64";
				GENERATE_INFOPLIST_FILE = YES;
				HEADER_SEARCH_PATHS = "$(PROJECT_DIR)/lib/uv/include";
				INFOPLIST_FILE = Info.plist;
				INFOPLIST_KEY_CFBundleDisplayName = "{{name}}";
				INFOPLIST_KEY_LSApplicationCategoryType = Developer;
				INFOPLIST_KEY_NSCameraUsageDescription = "This app needs access to the camera";
				INFOPLIST_KEY_NSHumanReadableCopyright = "{{copyright}}";
				INFOPLIST_KEY_NSMicrophoneUsageDescription = "This app needs access to the microphone";
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
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
				LIBRARY_SEARCH_PATHS = (
					"$(PROJECT_DIR)/lib",
					"$(PROJECT_DIR)/lib/uv",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = tc.operator.op;
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "op iOS AdHoc";
				SWIFT_EMIT_LOC_STRINGS = YES;
				TARGETED_DEVICE_FAMILY = 1;
			};
			name = Debug;
		};
		29124C7B2761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ARCHS = arm64;
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				ASSETCATALOG_COMPILER_GLOBAL_ACCENT_COLOR_NAME = AccentColor;
				CODE_SIGN_ENTITLEMENTS = op.entitlements;
				CODE_SIGN_IDENTITY = "iPhone Distribution";
				CODE_SIGN_STYLE = Manual;
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = Z3M838H537;
				ENABLE_BITCODE = NO;
				"EXCLUDED_ARCHS[sdk=*]" = "";
				"EXCLUDED_ARCHS[sdk=iphonesimulator*]" = x86_64;
				GENERATE_INFOPLIST_FILE = YES;
				HEADER_SEARCH_PATHS = "$(PROJECT_DIR)/lib/uv/include";
				INFOPLIST_FILE = Info.plist;
				INFOPLIST_KEY_CFBundleDisplayName = "{{name}}";
				INFOPLIST_KEY_LSApplicationCategoryType = Developer;
				INFOPLIST_KEY_NSCameraUsageDescription = "This app needs access to the camera";
				INFOPLIST_KEY_NSHumanReadableCopyright = "{{copyright}}";
				INFOPLIST_KEY_NSMicrophoneUsageDescription = "This app needs access to the microphone";
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
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
				LIBRARY_SEARCH_PATHS = (
					"$(PROJECT_DIR)/lib",
					"$(PROJECT_DIR)/lib/uv",
				);
				MARKETING_VERSION = 1.0;
				ONLY_ACTIVE_ARCH = YES;
				PRODUCT_BUNDLE_IDENTIFIER = tc.operator.op;
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "op iOS AdHoc";
				SWIFT_EMIT_LOC_STRINGS = YES;
				TARGETED_DEVICE_FAMILY = 1;
			};
			name = Release;
		};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		29124C4527613369001832A0 /* Build configuration list for PBXProject "op" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				29124C772761336B001832A0 /* Debug */,
				29124C782761336B001832A0 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		29124C792761336B001832A0 /* Build configuration list for PBXNativeTarget "op" */ = {
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
	<string>{{apple_distribution_method}}</string>
	<key>teamID</key>
	<string>{{apple_team_id}}</string>
	<key>uploadBitcode</key>
	<true/>
	<key>compileBitcode</key>
	<true/>
	<key>uploadSymbols</key>
	<true/>
	<key>signingStyle</key>
	<string>manual</string>
	<key>signingCertificate</key>
	<string>{{apple_signing_certificate}}</string>
	<key>provisioningProfiles</key>
	<dict>
		<key>{{bundle_identifier}}</key>
		<string>{{apple_provisioning_profile}}</string>
	</dict>
</dict>
</plist>)XML";

constexpr auto gXCodePlist = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleIconFile</key>
	<string>ui/icon.png</string>
	<key>NSAppTransportSecurity</key>
	<dict>
		<key>NSAllowsArbitraryLoads</key>
		<true/>
	</dict>
	<key>NSHighResolutionCapable</key>
	<true/>
	<key>NSLocalNetworkUsageDescription</key>
	<string>The app would like to connect to peers</string>
	<key>NSRequiresAquaSystemAppearance</key>
	<false/>
	<key>NSSupportsAutomaticGraphicsSwitching</key>
	<true/>
	<key>UIBackgroundModes</key>
	<array>
		<string>fetch</string>
		<string>processing</string>
	</array>
</dict>
</plist>)XML";

constexpr auto gXcodeEntitlements = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>com.apple.security.app-sandbox</key>
	<true/>
	<key>com.apple.security.network.server</key>
	<true/>
	<key>com.apple.security.network.client</key>
	<true/>
</dict>
</plist>)XML";

//
// Template generated by XCode.
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
               BuildableName = "op.app"
               BlueprintName = "op"
               ReferencedContainer = "container:op.xcodeproj">
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
            BuildableName = "op.app"
            BlueprintName = "op"
            ReferencedContainer = "container:op.xcodeproj">
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
            BuildableName = "op.app"
            BlueprintName = "op"
            ReferencedContainer = "container:op.xcodeproj">
         </BuildableReference>
      </BuildableProductRunnable>
   </ProfileAction>
   <AnalyzeAction
      buildConfiguration = "Debug">
   </AnalyzeAction>
   <ArchiveAction
      buildConfiguration = "Release"
      customArchiveName = "op"
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

constexpr auto gDefaultConfig = R"CONFIG(
#   ___  ___  ___ ___  ___ ____ ___  ___
#  /  / /__/ /_  /__/ /__/  /  /  / /__/
# /__/ /    /__ /  \ /  /  /  /__/ /  \
#
# Default configuration file. Delete what you don't need.
#

# This is made available for cross-compiling. It specifies the target
# architecture, by default it detects the machine on which the cli is
# executing.
# arch: x86_64

# Shell command to build an application.
build: node scripts/build.js

# A unique ID that identifies the bundle (used by all app stores).
bundle_identifier: com.beepboop

# A unique ID that identifies the bundle (used by all app stores).
bundle_identifier_short: boop

# A string that gets used in the about dialog and package meta info.
copyright: (c) Beep Boop Corp. 1985

# Advanced Compiler Settings for debug purposes (ie C++ compiler -g, etc).
debug_flags: -g -O3

# A short description of the app
description: A UI for the beep boop network

# An array of environment variables, separated by commas
# env: USER, TMPDIR, PWD

# The name of the product (ie Beep or Boop).
executable: boop

# Advanced Compiler Settings (ie C++ compiler -02, -03, etc).
flags: -O1

# A boolean that determines if stdout and stderr should get forwarded
# forward_console: true

# The initial height of the first window.
# height: 750

# A directory is where your application's code is located.
# input: src

# Localization
# lang: en-us

# A String used in the about dialog and meta info.
# maintainer: Beep Boop Corp.

# The name of the program
name: beepboop

# The binary output path
output: build

# The initial title of the window (can have spaces and symbols etc).
title: Beep Boop

# A string that indicates the version of the cli tool and resources.
version v0.0.1

# A string that indicates the version for MacOS.
version_short 0.0.1

# The initial width of the first window.
# width: 1024

# TODO: maybe the user doesn't need to know about this?
# revision: 123

#
# Windows
#

# The command to execute to spawn the “back-end” process.
# win_cmd: beepboop.exe

# The icon to use for identifying your app on Windows.
# win_icon:

# The icon to use for identifying your app on Windows.
# win_logo: src/icons/icon.png

# A relative path to the pfx file used for signing.
# win_pfx: certs/cert.pfx

# TODO description & value
# win_publisher: CN=Beep Boop Corp., O=Beep Boop Corp., L=San Francisco, S=California, C=US

#
# Linux
#

# Helps to make your app searchable in Linux desktop environments.
# linux_categories: Developer Tools

# The command to execute to spawn the ‚Äúback-end‚Äù process.
# linux_cmd: beepboop

# The icon to use for identifying your app in Linux desktop environments.
# linux_icon: src\icon.png

#
# MacOS
#

# Mac App Store icon
# mac_appstore_icon: src/icons/icon.png

# A category in the App Store
# mac_category:

# The command to execute to spawn the ‚Äúback-end‚Äù process.
# mac_cmd:

# TODO description & value
# mac_distribution_method:

# TODO description & value
# mac_entitlements:

# The icon to use for identifying your app on MacOS.
# mac_icon:

# TODO description & value
# mac_provisioning_profile:

# TODO description & value
# mac_sign:

# TODO description & value
# mac_signing_certificate:

# TODO description & value
# mac_sign_paths:

# TODO description & value
# mac_team_id:

#
# iOS
#

# which device to target when building for the simulator
# ios_device_simulator:

# A relative path to the plist of application entitlements
# ios_entitlements:

# TODO description & value
# ios_distribution_method:

# The provisioning profile that is used for signing (should be mac?)
# ios_provisioning_profile:

# TODO description & value
# ios_signing_certificate:

# TODO description & value
# ios_team_id:
)CONFIG";
