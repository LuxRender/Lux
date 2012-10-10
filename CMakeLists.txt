###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of Lux.                                             #
#                                                                         #
#   Lux is free software; you can redistribute it and/or modify           #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   Lux is distributed in the hope that it will be useful,                #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
#   Lux website: http://www.luxrender.net                                 #
###########################################################################
SET(LUXRAYS_DISABLE_OPENCL ON)
MESSAGE(STATUS "CMAKE VERSION DETECTED " ${CMAKE_VERSION})
IF(MSVC)
	cmake_minimum_required(VERSION 2.8)
	cmake_policy(VERSION 2.8)
ELSEIF(APPLE)
	cmake_minimum_required(VERSION 2.8.3) # new requirement for simplification
	cmake_policy(VERSION 2.8.3)
ELSE(MSVC)
	cmake_minimum_required(VERSION 2.4)
ENDIF(MSVC)    

IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

PROJECT(lux)
SET(VERSION 0.9)

SET(LUX_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} 
	"${LUX_CMAKE_DIR}"
	"${LUX_CMAKE_DIR}/SpecializedConfig"
)
option(LUXRAYS_DISABLE_OPENCL "Build without OpenCL support" ON)
option(LUX_DOCUMENTATION "Generate project documentation" ON)

# Dade - uncomment to obtain verbose building output
#SET(CMAKE_VERBOSE_MAKEFILE true)

# Don't overconfigure
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limit configs" FORCE)
# When using single configuration generators like make
# CMake does need to know which of the possible configurations
# to generate
IF (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)
MESSAGE(STATUS "Building mode: " ${CMAKE_BUILD_TYPE})

IF (LUXRAYS_DISABLE_OPENCL)
	ADD_DEFINITIONS("-DLUXRAYS_DISABLE_OPENCL")
ENDIF()

#############################################################################
#############################################################################
###                OSX general settings and options                       ###
#############################################################################
#############################################################################

#Setting Universal Binary Properties and Pylux configuration, only for Mac OS X
# generate with xcode/crosscompile, setting: ( darwin - 10.x - gcc - g++ - MacOSX10.x.sdk - Find from root, then native system )
IF(APPLE)

	########## OS and hardware detection ###########

	if(NOT ${CMAKE_GENERATOR} MATCHES "Xcode") # unix makefile generator does not fill XCODE_VERSION var ! 
		execute_process(COMMAND xcodebuild -version OUTPUT_VARIABLE XCODE_VERS_BUILDNR )
		STRING(SUBSTRING ${XCODE_VERS_BUILDNR} 6 3 XCODE_VERSION) # truncate away build-nr
	endif()

	if(${XCODE_VERSION} GREATER 4.0)
		set(OSX_OPTION_XCODE_4.1 ON)
	else()
		set(OSX_OPTION_XCODE_4.1 OFF)
	endif()
	execute_process(COMMAND uname -r OUTPUT_VARIABLE MAC_SYS)
	if (${MAC_SYS} MATCHES 12)
		set(OSX_SYSTEM 10.8 (MountainLion))
	elseif (${MAC_SYS} MATCHES 11)
		set(OSX_SYSTEM 10.7 (Lion))
	elseif(${MAC_SYS} MATCHES 10)
		set(OSX_SYSTEM 10.6 (SnowLeopard))
	else()
		set(OSX_SYSTEM unsupported)
	endif()
	execute_process(COMMAND sysctl hw.model OUTPUT_VARIABLE MAC_HW OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(SUBSTRING ${MAC_HW} 10 -1 MAC_MODEL)

	execute_process(COMMAND sysctl hw.optional.sse4_2 OUTPUT_VARIABLE HW_SSE_4_2 )
	if (${HW_SSE_4_2} MATCHES "hw.optional.sse4_2: 1")
		set(HAS_SSE 4.2)
	else()
		execute_process(COMMAND sysctl hw.optional.sse4_1 OUTPUT_VARIABLE HW_SSE_4_1 )
		if (${HW_SSE_4_1} MATCHES "hw.optional.sse4_1: 1")
			set(HAS_SSE 4.1)
		else()
			set(HAS_SSE 3)		
		endif()
	endif()
	
	########## OSX_OPTIONS ###########
	option(OSX_OPTION_PYLUX "Build a blender compatible pylux" ON)
	option(OSX_OPTION_CLANG "Build with CLANG compiler ( XCODE4 )" ON)
	option(OSX_OPTION_LTO "Build with LINK TIME OPTIMISATION ( MAY BREAK NON-SSE4 MACS COMPATIBILITY )" OFF)
	option(OSX_OPTION_USE_MAX_SSE "Build with highest SSE available on machine ( SSE_${HAS_SSE} )" OFF)
	option(OSX_OPTION_UNIVERSAL "Force compile universal" OFF)
	###################################

	SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.6) # keep this @ 10.6 to archieve bw-compatibility by weak-linking !

	if(${XCODE_VERSION} VERSION_LESS 4.3)
		SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.6.sdk)
	elseif(${XCODE_VERSION} VERSION_GREATER 4.3)
		set(CMAKE_XCODE_ATTRIBUTE_SDKROOT macosx10.8) # xcode 4.4-style
	else()
		SET(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk)
	endif()			

	if(OSX_OPTION_USE_MAX_SSE)
		if(HAS_SSE MATCHES 4.2)
			SET(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_SSE42_EXTENSIONS YES)
			SET(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_SSE41_EXTENSIONS YES)
		elseif(HAS_SSE MATCHES 4.1)
			SET(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_SSE41_EXTENSIONS YES)
		endif()
	endif()

	SET(CMAKE_XCODE_ATTRIBUTE_ARCHS i386\ x86_64) # valid arches
	SET(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING YES) # strip symbols in whole project, disabled in pylux target
	if(OSX_OPTION_UNIVERSAL)
		set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
	endif(OSX_OPTION_UNIVERSAL)
	if(OSX_OPTION_CLANG)
		SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION com.apple.compilers.llvm.clang.1_0)
	endif(OSX_OPTION_CLANG)
	if(OSX_OPTION_LTO)
		SET(CMAKE_XCODE_ATTRIBUTE_LLVM_LTO YES)
	endif(OSX_OPTION_LTO)
	if(NOT ${CMAKE_GENERATOR} MATCHES "Xcode") # will be set later in XCode
		SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "assure config" FORCE)
		# Setup binaries output directory in Xcode manner
		SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} CACHE PATH "per configuration" FORCE)
		SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE} CACHE PATH "per configuration" FORCE)
	else() # replace CMAKE_BUILD_TYPE with XCode env var $(CONFIGURATION) globally
		SET(CMAKE_BUILD_TYPE "$(CONFIGURATION)" )
	endif()
	set(OSX_DEPENDENCY_ROOT ${CMAKE_SOURCE_DIR}/../macos) # can be macos or usr/local for example
	set(OSX_BUNDLE_COMPONENTS_ROOT ${CMAKE_SOURCE_DIR}/../macos) # bundle resources
	INCLUDE(macosx_bundle)

ENDIF(APPLE)


#############################################################################
#############################################################################
### check for the CPU we build for                                        ###
#############################################################################
#############################################################################

IF(NOT APPLE AND NOT WIN32)
	EXECUTE_PROCESS(
		COMMAND ${CMAKE_C_COMPILER} -dumpmachine
		OUTPUT_VARIABLE MACHINE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	MESSAGE(STATUS "Building for target ${MACHINE}")

	STRING(REGEX MATCH "(i.86-*)|(athlon-*)|(pentium-*)" _mach_x86 ${MACHINE})
	IF (_mach_x86)
		SET(ARCH_X86 1)
	ENDIF (_mach_x86)

	STRING(REGEX MATCH "(x86_64-*)|(X86_64-*)|(AMD64-*)|(amd64-*)" _mach_x86_64 ${MACHINE})
	IF (_mach_x86_64)
		SET(ARCH_X86_64 1)
		SET(LIB_SUFFIX 64)
		#jromang - Hack to avoid boost bug on x64  Ubuntu 8.10 and Fedora 10 (http://www.luxrender.net/mantis/view.php?id=433)
		ADD_DEFINITIONS(-DBOOST_NO_INTRINSIC_INT64_T)
	ENDIF (_mach_x86_64)

	STRING(REGEX MATCH "(ppc-*)|(powerpc-*)" _mach_ppc ${MACHINE})
	IF (_mach_ppc)
		SET(ARCH_PPC 1)
	ENDIF (_mach_ppc)
ELSEIF(MSVC)
	STRING(REGEX MATCH "(Win64)" _carch_x64 ${CMAKE_GENERATOR})
	IF(_carch_x64)
		SET(ARCH_X86_64 1)
		MESSAGE(STATUS "Building for target x64")
	ELSE(_carch_x64)
		SET(ARCH_X86 1)
		MESSAGE(STATUS "Building for target Win32")
	ENDIF(_carch_x64)
ENDIF(NOT APPLE AND NOT WIN32)

#
# Overwrite defaults with Custom Settings
#
IF(LUX_CUSTOM_CONFIG)
	MESSAGE(STATUS "Using custom build config: ${LUX_CUSTOM_CONFIG}")
	INCLUDE(${LUX_CUSTOM_CONFIG})
ENDIF(LUX_CUSTOM_CONFIG)

#############################################################################
#############################################################################
######################    FIND PROJECT DEPENDENCIES    ######################
#############################################################################
#############################################################################
INCLUDE(Dependencies)

#############################################################################
#All dependencies OK !
#############################################################################

#############################################################################
#############################################################################
############################    DOCUMENTATION    ############################
#############################################################################
#############################################################################

IF(LUX_DOCUMENTATION)
	INCLUDE(documentation)
ENDIF(LUX_DOCUMENTATION)

#Generate the config.h file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
ADD_DEFINITIONS(-DLUX_USE_CONFIG_H)

#############################################################################
#############################################################################
#########################      COMPILER FLAGS     ###########################
#############################################################################
#############################################################################

IF(APPLE)
	# Jens - default Mac compiler options
	ADD_DEFINITIONS(-fvisibility=hidden -fvisibility-inlines-hidden -Wall -DHAVE_PTHREAD_H) # global compile definitions
	set(OSX_FLAGS_RELEASE "-ftree-vectorize -msse -msse2 -msse3 -mssse3 -fvariable-expansion-in-unroller") # only additional flags
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OSX_FLAGS_RELEASE}") # cmake emits "-O3 -DNDEBUG" for Release by default, "-O0 -g" for Debug
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OSX_FLAGS_RELEASE}")
ELSEIF(MSVC)
	# Default MSVC compiler options
	MESSAGE(STATUS "MSVC")

	SET(GUI_TYPE WIN32)

	# Change warning to something saner
	#Force to always compile with W3
	IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		STRING(REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	ELSE()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
	ENDIF()

	IF(MSVC90)
		MESSAGE(STATUS "Version 9")
		# Whole Program Opt. gui display fixed in cmake 2.8.5
		# See http://public.kitware.com/Bug/view.php?id=6794
		# /GL will be used to build the code but the selection is not displayed in the menu

		# /Qfast_transcendentals causes inlining of those functions but no loss of precision

		SET(MSVC_RELEASE_COMPILER_FLAGS "/O2 /Ob1 /Oi /Oy- /GT /GL /EHsc /MD /GS /fp:precise /Zc:forScope /GR /Gy /Gd /TP /Ot /favor:blend /Qfast_transcendentals /MP")
		SET(MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS "/Zi")

		IF(NOT ARCH_X86_64)
			SET(MSVC_RELEASE_COMPILER_FLAGS ${MSVC_RELEASE_COMPILER_FLAGS} " /arch:SSE2")
		ENDIF(NOT ARCH_X86_64)

		SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${MSVC_RELEASE_COMPILER_FLAGS} ${MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS}")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_RELEASE_COMPILER_FLAGS} ${MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS}")
	ELSEIF(MSVC10)
		MESSAGE(STATUS "Version 10")
		# Whole Program Opt. gui display fixed in cmake 2.8.5
		# See http://public.kitware.com/Bug/view.php?id=6794
		# /GL will be used to build the code but the selection is not displayed in the menu

		# /Qfast_transcendentals causes inlining of those functions but no loss of precision

		SET(MSVC_RELEASE_COMPILER_FLAGS "/WX- /MP /O2 /Ob1 /Oi /Oy- /GT /GL /Gm- /EHsc /MD /GS /fp:precise /Zc:forScope /GR /Gy /openmp /Gd /TP /GL /GF- /Ot /favor:blend /Qfast_transcendentals")
		SET(MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS "/Zi")

		SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${MSVC_RELEASE_COMPILER_FLAGS} ${MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS}")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MSVC_RELEASE_COMPILER_FLAGS} ${MSVC_RELEASE_WITH_DEBUG_COMPILER_FLAGS}")
	ENDIF(MSVC90)

	# Definitions
	ADD_DEFINITIONS(-D__SSE2__ -D__SSE__ -D__MMX__)
	ADD_DEFINITIONS(-D_UNICODE -DUNICODE)
	ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
	
ELSE(APPLE)
	# Dade - default compiler options
	# Removed "-O3" from definitions in order to have CMake Release/Debug options to work
	ADD_DEFINITIONS(-msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -fPIC -DHAVE_PTHREAD_H)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fvisibility=hidden -fvisibility-inlines-hidden")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden -fvisibility-inlines-hidden")
	# The QBVH accelerator needs to be compiled with much reduced optimizations
	# otherwise gcc produces incorrect code and ruins the render on 64bits machines
	SET_SOURCE_FILES_PROPERTIES(accelerators/qbvhaccel.cpp COMPILE_FLAGS "-O1")
ENDIF(APPLE)

#############################################################################
# Compiler flags for specific setup
#############################################################################

# Dade - GCC Profiling (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-pg -g -O2 -msse2 -mfpmath=sse -ftree-vectorize -Wall -DHAVE_PTHREAD_H)

# Dade - GCC 2 pass optimization (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-O3 --coverage -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -fbranch-probabilities -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DHAVE_PTHREAD_H)

# Dade - my settings
#ADD_DEFINITIONS(-g -O0 -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=prescott -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -m3dnow -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DHAVE_PTHREAD_H )

# Dade - Intel CC settings (double pass, 32bit, remember to uncomment the line at the end of file too)
#  rm -rf CMakeCache.txt CMakeFiles
#  CC=/opt/intel/cc/10.1.015/bin/icc CXX=/opt/intel/cc/10.1.015/bin/icpc cmake lux
# Pass 1
#ADD_DEFINITIONS(-prof-gen -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
# Pass 2
#ADD_DEFINITIONS(-prof-use -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
 
# Dade - Intel CC settings (single pass, 32bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')

# Dade - Intel CC settings (single pass, 64bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"' -DBOOST_NO_INTRINSIC_INT64_T "-D'__builtin_vsnprintf(__out, __size, __fmt, __args)'='__builtin_vsnprintf(__out, __size, __fmt, (char *) __args)'") 

#ADD_DEFINITIONS(-O3 -msse -mfpmath=sse -ftree-vectorize -Wall -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -mmmx -m3dnow -msse -mfpmath=sse -ftree-vectorize -Wall -DHAVE_PTHREAD_H )
#ADD_DEFINITIONS(-O3 -Wall -msse -msse2 -DLUX_USE_SSE -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DHAVE_PTHREAD_H)
#ICC definitions
#ADD_DEFINITIONS(-O3 -ipo -no-prec-div -static -fp-model fast=2 -rcd)

#############################################################################
#############################################################################
###########################   SYSTEM LIBRARIES    ###########################
#############################################################################
#############################################################################
IF(APPLE)
	SET(SYS_LIBRARIES z bz2)
ELSEIF (WIN32)
	SET(SYS_LIBRARIES ${SYS_LIBRARIES} "shell32.lib")
ENDIF (APPLE)

#############################################################################
#############################################################################
#####################            LINKER INFO           ######################
#############################################################################
#############################################################################

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS} )
IF (APPLE)
	SET(OSX_SHARED_CORELIB ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/liblux.dylib)
	ADD_DEFINITIONS(-DLUX_DLL) # define we use dynamic corelib
ELSEIF(MSVC)
	IF(MSVC90)
		# LTCG not correctly set by cmake in the current release - in the meantime the linker will inform you about switching this flag automatically because of /GL
		#SET(MSVC_RELEASE_LINKER_FLAGS "/LTCG")
		SET(MSVC_RELEASE_LINKER_FLAGS "/LTCG /OPT:NOREF /OPT:ICF")
		SET(MSVC_RELEASE_WITH_DEBUG_LINKER_FLAGS "/DEBUG")
		SET(STATIC_LIBRARY_FLAGS_RELEASE "${STATIC_LIBRARY_FLAGS_RELEASE} ${MSVC_RELEASE_LINKER_FLAGS} ${MSVC_DEBUG_LINKER_FLAGS}")
		SET(SHARED_LIBRARY_FLAGS_RELEASE "${SHARED_LIBRARY_FLAGS_RELEASE} ${MSVC_RELEASE_LINKER_FLAGS} ${MSVC_DEBUG_LINKER_FLAGS}")
		SET(CMAKE_EXE_LINKER_FLAGS "${MSVC_RELEASE_LINKER_FLAGS}")

		# Prevent cmake from adding about any std. lib there is
		SET(CMAKE_C_STANDARD_LIBRARIES "")
		SET(CMAKE_CXX_STANDARD_LIBRARIES "")
	ELSEIF(MSVC10)
		# LTCG not correctly set by cmake in the current release - in the meantime the linker will inform you about switching this flag automatically because of /GL
		#SET(MSVC_RELEASE_LINKER_FLAGS "/LTCG")
		SET(MSVC_RELEASE_LINKER_FLAGS "/LTCG /OPT:REF /OPT:ICF")
		SET(MSVC_RELEASE_WITH_DEBUG_LINKER_FLAGS "/DEBUG")
		SET(STATIC_LIBRARY_FLAGS_RELEASE "${STATIC_LIBRARY_FLAGS_RELEASE} ${MSVC_RELEASE_LINKER_FLAGS} ${MSVC_DEBUG_LINKER_FLAGS}")
		SET(SHARED_LIBRARY_FLAGS_RELEASE "${SHARED_LIBRARY_FLAGS_RELEASE} ${MSVC_RELEASE_LINKER_FLAGS} ${MSVC_DEBUG_LINKER_FLAGS}")

		# Prevent cmake from adding about any std. lib there is
		SET(CMAKE_C_STANDARD_LIBRARIES "")
		SET(CMAKE_CXX_STANDARD_LIBRARIES "")
	ENDIF(MSVC90)
	ADD_DEFINITIONS(-DLUX_DLL)
	SET(LUX_LIBRARY luxShared)
ELSE(APPLE)
	SET(LUX_LIBRARY luxShared)
	ADD_DEFINITIONS(-DLUX_DLL) # for controlling visibility
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	SET(CMAKE_INSTALL_RPATH "$ORIGIN")
ENDIF(APPLE)
SET(LUX_LIBRARY_DEPENDS ${LUXRAYS_LIBRARY} ${OPENCL_LIBRARIES} ${OPENGL_LIBRARY} ${FREEIMAGE_LIBRARIES} ${Boost_LIBRARIES} ${SYS_LIBRARIES})

#############################################################################
#############################################################################
#########################      LIBRARY BUILD      ###########################
#############################################################################
#############################################################################
INCLUDE(liblux)

#############################################################################
#############################################################################
############################   PYTHON LIBRARY    ############################
#############################################################################
#############################################################################
INCLUDE(pylux)

#############################################################################
#############################################################################
#############################    Binaries       #############################
#############################################################################
#############################################################################
INCLUDE(luxconsole)
INCLUDE(luxmerger)
INCLUDE(luxcomp)
INCLUDE(luxrender)

#############################################################################
#############################################################################
##########################   Install targets       ##########################
#############################################################################
#############################################################################

IF(APPLE)

#	TODO: custom install

ELSEIF(WIN32)

	#TODO: custom install

ELSE(APPLE)
	INSTALL(TARGETS luxconsole luxmerger RUNTIME DESTINATION bin)
	IF(QT4_FOUND)
		INSTALL(TARGETS luxrender RUNTIME DESTINATION bin)
	ENDIF(QT4_FOUND)

	#Install API/Library
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/core/api.h DESTINATION include/luxrender/)
	INSTALL(TARGETS luxShared DESTINATION lib${LIB_SUFFIX})

	#Install Desktop files
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/luxrender.svg DESTINATION share/pixmaps/)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/luxrender.desktop DESTINATION share/applications/)

	#Source package target
	ADD_CUSTOM_TARGET(package mkdir lux-${VERSION}
		COMMAND cp -R ${CMAKE_SOURCE_DIR}/* lux-${VERSION}
		COMMAND tar -cf ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar lux-${VERSION}
		COMMAND bzip2 --best ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		DEPENDS ${lux_lib_src}
		COMMENT Building source package
	)
ENDIF(APPLE)
# Dade - GCC Profiling (remember to uncomment the line in the middle of file too)
#SET_TARGET_PROPERTIES(luxconsole luxmerger luxrender PROPERTIES LINK_FLAGS "-pg")

# Dade - GCC 2 pass optimization (remember to uncomment the line in the middle of file too)
#SET_TARGET_PROPERTIES(luxconsole luxmerger luxrender PROPERTIES LINK_FLAGS "--coverage")

# Dade - Intel Compiler optimization
#REMOVE_DEFINITIONS(-ipo)

IF(APPLE)
	MESSAGE(STATUS "")
	MESSAGE(STATUS "################ GENERATED OSX PROJECT INFORMATION ################")
	MESSAGE(STATUS "")
	MESSAGE(STATUS "GENERATED_PROJECT_TYPE : " ${CMAKE_GENERATOR})
	MESSAGE(STATUS "")
	MESSAGE(STATUS "BUILD_MACHINE_SYS : OSX_" ${OSX_SYSTEM})
	MESSAGE(STATUS "BUILD_MACHINE_MODEL : " ${MAC_MODEL})
	MESSAGE(STATUS "BUILD_MACHINE_MAX_SSE_SUPPORTED : SSE_"${HAS_SSE})
	MESSAGE(STATUS "")
	MESSAGE(STATUS "OSX_DEPLOYMENT_TARGET : " ${CMAKE_OSX_DEPLOYMENT_TARGET})
	MESSAGE(STATUS "OSX SDK SETTING : " ${CMAKE_OSX_SYSROOT})
	MESSAGE(STATUS "OSX_DEPENDENCY_ROOT_PATH : " ${OSX_DEPENDENCY_ROOT})
	MESSAGE(STATUS "OSX_BUILD_WITH_CLANG : " ${OSX_OPTION_CLANG})
	MESSAGE(STATUS "OSX_USE_LTO : " ${OSX_OPTION_LTO})
	MESSAGE(STATUS "")
	MESSAGE(STATUS "DETECTED_XCODE_VERSION : " ${XCODE_VERSION})
	MESSAGE(STATUS "USE_XCODE_4.1_FEATURES : " ${OSX_OPTION_XCODE_4.1})
	MESSAGE(STATUS "VALID_BUILD_ARCHTECTURES : " ${CMAKE_XCODE_ATTRIBUTE_ARCHS})
	MESSAGE(STATUS "")
	if(${CMAKE_GENERATOR} MATCHES "Xcode")
		MESSAGE(STATUS "BUILD_TYPE : Please set in Xcode DYNAMIC_BUILD target to aimed type")
	else()
		MESSAGE(STATUS "BUILD_TYPE : " ${CMAKE_BUILD_TYPE} " - compile with: make DYNAMIC_BUILD" )
	endif()
	IF(OSX_OPTION_PYLUX)
		MESSAGE(STATUS "PYLUX CONFIGURED FOR BLENDER 2.6 USE")
	ELSE(OSX_OPTION_PYLUX)
		MESSAGE(STATUS "PYLUX CONFIGURED FOR GENERAL PURPOSE USE")
	ENDIF(OSX_OPTION_PYLUX)
	MESSAGE(STATUS "USE_BEST_SSE_OPTIMIZATION : " ${OSX_OPTION_USE_MAX_SSE})
	MESSAGE(STATUS "ALWAYS_BUILD_UNIVERSAL : " ${OSX_OPTION_UNIVERSAL})
	IF(NOT OSX_OPTION_USE_MAX_SSE AND NOT OSX_OPTION_LTO AND ${CMAKE_BUILD_TYPE} MATCHES Release)
		MESSAGE(STATUS "COMPATIBILITY INFO : BUILD IS CONFIGURED DISTRIBUTION SAFE")
	ELSE()
		MESSAGE(STATUS "COMPATIBILITY INFO : BUILD SETTINGS DO NOT SUPPORT ALL INTEL-MACS")
	ENDIF()
	MESSAGE(STATUS "")
	MESSAGE(STATUS "#####################################################################")
ENDIF(APPLE)
