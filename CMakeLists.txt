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

cmake_minimum_required(VERSION 2.4)
IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)
MESSAGE(STATUS "CMAKE VERSION DETECTED " ${CMAKE_VERSION})

PROJECT(lux)
SET(VERSION 0.9)

SET(LUX_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LUX_CMAKE_DIR}")
option(LUXRAYS_DISABLE_OPENCL "Build without OpenCL support" OFF)
option(LUX_DOCUMENTATION "Generate project documentation" ON)

# Dade - uncomment to obtain verbose building output
#SET(CMAKE_VERBOSE_MAKEFILE true)

#############################################################################
#############################################################################
###                OSX general settings and options                       ###
#############################################################################
#############################################################################

#Setting Universal Binary Properties and Pylux configuration, only for Mac OS X
# generate with xcode/crosscompile, setting: ( darwin - 10.6 - gcc - g++ - MacOSX10.6.sdk - Find from root, then native system )
IF(APPLE)
	########## OSX_OPTIONS ###########
	option(OSX_OPTION_PYLUX "Build a blender compatible pylux" ON)
	option(OSX_OPTION_CLANG "Build with CLANG compiler ( XCODE4 )" ON)
	option(OSX_OPTION_UNIVERSAL "Force compile universal" OFF)
	option(OSX_OPTION_DYNAMIC_BUILD "Link lux apps to shared corelib" ON)
	###################################
	SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)
	IF(CMAKE_VERSION VERSION_LESS 2.8.1)
		SET(CMAKE_OSX_ARCHITECTURES i386;x86_64) # valid arches
	ELSE(CMAKE_VERSION VERSION_LESS 2.8.1)
		SET(CMAKE_XCODE_ATTRIBUTE_ARCHS i386\ x86_64) # valid arches
		SET(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING YES) # strip symbols
	ENDIF(CMAKE_VERSION VERSION_LESS 2.8.1)
	if(OSX_OPTION_UNIVERSAL)
		set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
	endif(OSX_OPTION_UNIVERSAL)
	SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.6.sdk)
	set(CMAKE_CONFIGURATION_TYPES Release)
	set(OSX_DEPENDENCY_ROOT ${CMAKE_SOURCE_DIR}/../macos) # can be macos or usr/local for example
	SET(OSX_SHARED_CORELIB ${OSX_DEPENDENCY_ROOT}/lib/corelib/liblux.dylib) # this is just a safety net
ENDIF(APPLE)


#############################################################################
#############################################################################
### check for the CPU we build for                                        ###
#############################################################################
#############################################################################

IF(NOT APPLE)
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
ENDIF(NOT APPLE)

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
	ADD_DEFINITIONS(-O3 -ftree-vectorize -msse -msse2 -msse3 -mssse3 -fvariable-expansion-in-unroller -fpic -Wall -DHAVE_PTHREAD_H )
ELSE(APPLE)
	# Dade - default compiler options
	ADD_DEFINITIONS(-O3 -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -fPIC -DHAVE_PTHREAD_H)
	# The QBVH accelerator needs to be compiled with much reduced optimizations
	# otherwise gcc produces incorrect code and ruins the render on 64bits machines
	SET_SOURCE_FILES_PROPERTIES(accelerators/qbvhaccel.cpp COMPILE_FLAGS "-O1")
ENDIF(APPLE)

#############################################################################
# Compiler flags for specific setup
#############################################################################

# Jens - testing Mac compiler options (debug)
#ADD_DEFINITIONS(-O0 -fpic -Wall -DHAVE_PTHREAD_H)

# Dade - GCC Profiling (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-pg -g -O2 -msse2 -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)

# Dade - GCC 2 pass optimization (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-O3 --coverage -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -fbranch-probabilities -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)

# Dade - my settings
#ADD_DEFINITIONS(-g -O0 -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=prescott -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -m3dnow -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )

# Dade - Intel CC settings (double pass, 32bit, remember to uncomment the line at the end of file too)
#  rm -rf CMakeCache.txt CMakeFiles
#  CC=/opt/intel/cc/10.1.015/bin/icc CXX=/opt/intel/cc/10.1.015/bin/icpc cmake lux
# Pass 1
#ADD_DEFINITIONS(-prof-gen -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
# Pass 2
#ADD_DEFINITIONS(-prof-use -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
 
# Dade - Intel CC settings (single pass, 32bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')

# Dade - Intel CC settings (single pass, 64bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"' -DBOOST_NO_INTRINSIC_INT64_T "-D'__builtin_vsnprintf(__out, __size, __fmt, __args)'='__builtin_vsnprintf(__out, __size, __fmt, (char *) __args)'") 

#ADD_DEFINITIONS(-O3 -msse -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -mmmx -m3dnow -msse -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )
#ADD_DEFINITIONS(-O3 -Wall -msse -msse2 -DLUX_USE_SSE -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ICC definitions
#ADD_DEFINITIONS(-O3 -ipo -no-prec-div -static -fp-model fast=2 -rcd)

#############################################################################
#############################################################################
#####################            LINKER INFO           ######################
#############################################################################
#############################################################################

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS} )

#############################################################################
#############################################################################
###########################   SYSTEM LIBRARIES    ###########################
#############################################################################
#############################################################################
IF(APPLE)
	SET(SYS_LIBRARIES z bz2)
ENDIF(APPLE)

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

ELSE(APPLE)
	INSTALL(TARGETS luxconsole luxmerger RUNTIME DESTINATION bin)
	IF(QT4_FOUND)
		INSTALL(TARGETS luxrender RUNTIME DESTINATION bin)
	ENDIF(QT4_FOUND)

#Install API/Library
INSTALL(FILES ${CMAKE_SOURCE_DIR}/core/api.h DESTINATION include/luxrender/)
INSTALL(TARGETS luxStatic DESTINATION lib${LIB_SUFFIX})
#	ELSE(FIND_LIBRARY_USE_LIB64_PATHS) ARCHIVE DESTINATION lib
#	ENDIF(FIND_LIBRARY_USE_LIB64_PATHS)


#Install Desktop files
INSTALL(FILES ${CMAKE_SOURCE_DIR}/wxgui/luxrender.svg DESTINATION share/pixmaps/)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/wxgui/luxrender.desktop DESTINATION share/applications/)

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
	MESSAGE(STATUS "################ GENERATED XCODE PROJECT INFORMATION ################")
	MESSAGE(STATUS "")
	MESSAGE(STATUS "OSX_DEPLOYMENT_TARGET : " ${CMAKE_OSX_DEPLOYMENT_TARGET})
	IF(CMAKE_VERSION VERSION_LESS 2.8.1)
		MESSAGE(STATUS "Setting CMAKE_OSX_ARCHITECTURES ( cmake lower 2.8 method ): " ${CMAKE_OSX_ARCHITECTURES})
	ELSE(CMAKE_VERSION VERSION_LESS 2.8.1)
		MESSAGE(STATUS "CMAKE_XCODE_ATTRIBUTE_ARCHS ( cmake 2.8 or higher method ): " ${CMAKE_XCODE_ATTRIBUTE_ARCHS})
	ENDIF(CMAKE_VERSION VERSION_LESS 2.8.1)
	MESSAGE(STATUS "OSX SDK SETTING : " ${CMAKE_OSX_SYSROOT})
	MESSAGE(STATUS "OSX_DEPENDENCY_ROOT_PATH : " ${OSX_DEPENDENCY_ROOT})
	MESSAGE(STATUS "BUILD_CONFIGURATION_TYPE : " ${CMAKE_CONFIGURATION_TYPES})
	MESSAGE(STATUS "OSX_CORE_BUILD_WITH_CLANG : " ${OSX_OPTION_CLANG})
	IF(OSX_OPTION_PYLUX)
		MESSAGE(STATUS "PYLUX CONFIGURED FOR BLENDER 2.5 USE")
	ELSE(OSX_OPTION_PYLUX)
		MESSAGE(STATUS "PYLUX CONFIGURED FOR GENERAL PURPOSE USE")
	ENDIF(OSX_OPTION_PYLUX)
	MESSAGE(STATUS "ALWAYS_BUILD_UNIVERSAL : " ${OSX_OPTION_UNIVERSAL})
	MESSAGE(STATUS "USE SHARED CORELIB : " ${OSX_OPTION_DYNAMIC_BUILD})
	MESSAGE(STATUS "")
	MESSAGE(STATUS "#####################################################################")
ENDIF(APPLE)