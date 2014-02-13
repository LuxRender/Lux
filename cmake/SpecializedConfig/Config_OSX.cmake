
###########################################################################
#
# Configuration ( Jens Verwiebe )
#
###########################################################################

MESSAGE(STATUS "Using OSX Configuration settings")

#Libs that have find_package modules
SET(FREEIMAGE_ROOT ${OSX_DEPENDENCY_ROOT})
set(OPENIMAGEIO_ROOT_DIR ${OSX_DEPENDENCY_ROOT})
set(OPENEXR_ROOT ${OSX_DEPENDENCY_ROOT})

#Libs with hardoded pathes ( macos repo )
set(TIFF_LIBRARIES ${OSX_DEPENDENCY_ROOT}/lib/libtiff.a)
set(TIFF_INCLUDE_DIR ${OSX_DEPENDENCY_ROOT}/include/tiff)
include_directories(SYSTEM ${TIFF_INCLUDE_DIR})
set(TIFF_FOUND ON)
set(JPEG_LIBRARIES ${OSX_DEPENDENCY_ROOT}/lib/libjpeg.a)
set(JPEG_INCLUDE_DIR ${OSX_DEPENDENCY_ROOT}/include/jpeg)
include_directories(SYSTEM ${JPEG_INCLUDE_DIR})
set(JPEG_FOUND ON)
SET(PNG_LIBRARIES ${OSX_DEPENDENCY_ROOT}/lib/libpng14.a)
SET(PNG_INCLUDE_DIR ${OSX_DEPENDENCY_ROOT}/include/png)
include_directories(SYSTEM ${PNG_INCLUDE_DIR})
SET(PNG_FOUND ON)