
constexpr auto gXCodeProjectTemplate = R"ASCII(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 55;
	objects = {

/* Begin PBXBuildFile section */
		29124C5827613369001832A0 /* Main.storyboard in Resources */ = {isa = PBXBuildFile; fileRef = 29124C5627613369001832A0 /* Main.storyboard */; };
		29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */ = {isa = PBXBuildFile; fileRef = 29124C5B2761336B001832A0 /* LaunchScreen.storyboard */; };
		29124C602761336B001832A0 /* main.m in Sources */ = {isa = PBXBuildFile; fileRef = 29124C5F2761336B001832A0 /* main.m */; };
		294A3C4C2763B8ED007B5B9A /* ui in Resources */ = {isa = PBXBuildFile; fileRef = 294A3C4B2763B8ED007B5B9A /* ui */; };
/* End PBXBuildFile section */

/* Begin PBXContainerItemProxy section */
		29124C662761336B001832A0 /* PBXContainerItemProxy */ = {
			isa = PBXContainerItemProxy;
			containerPortal = 29124C4227613369001832A0 /* Project object */;
			proxyType = 1;
			remoteGlobalIDString = 29124C4927613369001832A0;
			remoteInfo = opkit;
		};
		29124C702761336B001832A0 /* PBXContainerItemProxy */ = {
			isa = PBXContainerItemProxy;
			containerPortal = 29124C4227613369001832A0 /* Project object */;
			proxyType = 1;
			remoteGlobalIDString = 29124C4927613369001832A0;
			remoteInfo = opkit;
		};
/* End PBXContainerItemProxy section */

/* Begin PBXFileReference section */
		29124C4A27613369001832A0 /* opkit.app */ = {isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = opkit.app; sourceTree = BUILT_PRODUCTS_DIR; };
		29124C5727613369001832A0 /* Base */ = {isa = PBXFileReference; lastKnownFileType = file.storyboard; name = Base; path = Base.lproj/Main.storyboard; sourceTree = "<group>"; };
		29124C5C2761336B001832A0 /* Base */ = {isa = PBXFileReference; lastKnownFileType = file.storyboard; name = Base; path = Base.lproj/LaunchScreen.storyboard; sourceTree = "<group>"; };
		29124C5E2761336B001832A0 /* Info.plist */ = {isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; };
		29124C5F2761336B001832A0 /* main.m */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.objc; path = main.m; sourceTree = "<group>"; };
		294A3C4B2763B8ED007B5B9A /* ui */ = {isa = PBXFileReference; lastKnownFileType = folder; path = ui; sourceTree = "<group>"; };
		294A3C4D2763B949007B5B9A /* ui */ = {isa = PBXFileReference; lastKnownFileType = folder; path = ui; sourceTree = "<group>"; };
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		29124C4727613369001832A0 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C622761336B001832A0 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C6C2761336B001832A0 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		29124C4127613369001832A0 = {
			isa = PBXGroup;
			children = (
				294A3C4B2763B8ED007B5B9A /* ui */,
				294A3C4D2763B949007B5B9A /* ui */,
				29124C5627613369001832A0 /* Main.storyboard */,
				29124C5B2761336B001832A0 /* LaunchScreen.storyboard */,
				29124C5E2761336B001832A0 /* Info.plist */,
				29124C5F2761336B001832A0 /* main.m */,
				29124C4B27613369001832A0 /* Products */,
			);
			sourceTree = "<group>";
		};
		29124C4B27613369001832A0 /* Products */ = {
			isa = PBXGroup;
			children = (
				29124C4A27613369001832A0 /* opkit.app */,
			);
			name = Products;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

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
		29124C642761336B001832A0 /* opkitTests */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 29124C7C2761336B001832A0 /* Build configuration list for PBXNativeTarget "opkitTests" */;
			buildPhases = (
				29124C612761336B001832A0 /* Sources */,
				29124C622761336B001832A0 /* Frameworks */,
				29124C632761336B001832A0 /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
				29124C672761336B001832A0 /* PBXTargetDependency */,
			);
			name = opkitTests;
			productName = opkitTests;
			productType = "com.apple.product-type.bundle.unit-test";
		};
		29124C6E2761336B001832A0 /* opkitUITests */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 29124C7F2761336B001832A0 /* Build configuration list for PBXNativeTarget "opkitUITests" */;
			buildPhases = (
				29124C6B2761336B001832A0 /* Sources */,
				29124C6C2761336B001832A0 /* Frameworks */,
				29124C6D2761336B001832A0 /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
				29124C712761336B001832A0 /* PBXTargetDependency */,
			);
			name = opkitUITests;
			productName = opkitUITests;
			productType = "com.apple.product-type.bundle.ui-testing";
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
				29124C642761336B001832A0 /* opkitTests */,
				29124C6E2761336B001832A0 /* opkitUITests */,
			);
		};
/* End PBXProject section */

/* Begin PBXResourcesBuildPhase section */
		29124C4827613369001832A0 /* Resources */ = {
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				294A3C4C2763B8ED007B5B9A /* ui in Resources */,
				29124C5D2761336B001832A0 /* LaunchScreen.storyboard in Resources */,
				29124C5827613369001832A0 /* Main.storyboard in Resources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C632761336B001832A0 /* Resources */ = {
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C6D2761336B001832A0 /* Resources */ = {
			isa = PBXResourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXResourcesBuildPhase section */

/* Begin PBXSourcesBuildPhase section */
		29124C4627613369001832A0 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				29124C602761336B001832A0 /* main.m in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C612761336B001832A0 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
		29124C6B2761336B001832A0 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

/* Begin PBXTargetDependency section */
		29124C672761336B001832A0 /* PBXTargetDependency */ = {
			isa = PBXTargetDependency;
			target = 29124C4927613369001832A0 /* opkit */;
			targetProxy = 29124C662761336B001832A0 /* PBXContainerItemProxy */;
		};
		29124C712761336B001832A0 /* PBXTargetDependency */ = {
			isa = PBXTargetDependency;
			target = 29124C4927613369001832A0 /* opkit */;
			targetProxy = 29124C702761336B001832A0 /* PBXContainerItemProxy */;
		};
/* End PBXTargetDependency section */

/* Begin PBXVariantGroup section */
		29124C5627613369001832A0 /* Main.storyboard */ = {
			isa = PBXVariantGroup;
			children = (
				29124C5727613369001832A0 /* Base */,
			);
			name = Main.storyboard;
			sourceTree = "<group>";
		};
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
				INFOPLIST_FILE = Info.plist;
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
				PRODUCT_BUNDLE_IDENTIFIER = tc.operator.opkit;
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
				INFOPLIST_FILE = Info.plist;
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
				PRODUCT_BUNDLE_IDENTIFIER = tc.operator.opkit;
				PRODUCT_NAME = "$(TARGET_NAME)";
				PROVISIONING_PROFILE_SPECIFIER = "";
				SWIFT_EMIT_LOC_STRINGS = YES;
				TARGETED_DEVICE_FAMILY = "1,2";
			};
			name = Release;
		};
		29124C7D2761336B001832A0 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				BUNDLE_LOADER = "$(TEST_HOST)";
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				GENERATE_INFOPLIST_FILE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 15.0;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
					"@loader_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = test.opkitTests;
				PRODUCT_NAME = "$(TARGET_NAME)";
				SWIFT_EMIT_LOC_STRINGS = NO;
				TARGETED_DEVICE_FAMILY = "1,2";
				TEST_HOST = "$(BUILT_PRODUCTS_DIR)/opkit.app/opkit";
			};
			name = Debug;
		};
		29124C7E2761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				BUNDLE_LOADER = "$(TEST_HOST)";
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				GENERATE_INFOPLIST_FILE = YES;
				IPHONEOS_DEPLOYMENT_TARGET = 15.0;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
					"@loader_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = test.opkitTests;
				PRODUCT_NAME = "$(TARGET_NAME)";
				SWIFT_EMIT_LOC_STRINGS = NO;
				TARGETED_DEVICE_FAMILY = "1,2";
				TEST_HOST = "$(BUILT_PRODUCTS_DIR)/opkit.app/opkit";
			};
			name = Release;
		};
		29124C802761336B001832A0 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				GENERATE_INFOPLIST_FILE = YES;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
					"@loader_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = test.opkitUITests;
				PRODUCT_NAME = "$(TARGET_NAME)";
				SWIFT_EMIT_LOC_STRINGS = NO;
				TARGETED_DEVICE_FAMILY = "1,2";
				TEST_TARGET_NAME = opkit;
			};
			name = Debug;
		};
		29124C812761336B001832A0 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CODE_SIGN_STYLE = Automatic;
				CURRENT_PROJECT_VERSION = 1;
				GENERATE_INFOPLIST_FILE = YES;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
					"@loader_path/Frameworks",
				);
				MARKETING_VERSION = 1.0;
				PRODUCT_BUNDLE_IDENTIFIER = test.opkitUITests;
				PRODUCT_NAME = "$(TARGET_NAME)";
				SWIFT_EMIT_LOC_STRINGS = NO;
				TARGETED_DEVICE_FAMILY = "1,2";
				TEST_TARGET_NAME = opkit;
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
		29124C7C2761336B001832A0 /* Build configuration list for PBXNativeTarget "opkitTests" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				29124C7D2761336B001832A0 /* Debug */,
				29124C7E2761336B001832A0 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		29124C7F2761336B001832A0 /* Build configuration list for PBXNativeTarget "opkitUITests" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				29124C802761336B001832A0 /* Debug */,
				29124C812761336B001832A0 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
/* End XCConfigurationList section */
	};
	rootObject = 29124C4227613369001832A0 /* Project object */;
}
)ASCII";
