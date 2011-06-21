
###########################################################################
#
# Configuration
#
###########################################################################

MESSAGE(STATUS "Using Foobarbarian Configuration settings")

SET ( ENV{QTDIR}								"d:/qt/4.7.3"											)
SET ( ENV{OpenEXR_HOME}					"${lux_SOURCE_DIR}/../OpenEXR"		)
SET ( ENV{LuxRays_HOME}					"${lux_SOURCE_DIR}/../luxrays"		)


SET ( FREEIMAGE_SEARCH_PATH			"${lux_SOURCE_DIR}/../FreeImage"	)
SET ( FreeImage_INC_SEARCH_PATH	"${FREEIMAGE_SEARCH_PATH}/source"	)
SET ( FreeImage_LIB_SEARCH_PATH	"${FREEIMAGE_SEARCH_PATH}/release"
																	"${FREEIMAGE_SEARCH_PATH}/debug"
																	"${FREEIMAGE_SEARCH_PATH}/dist"	)

SET ( BOOST_SEARCH_PATH					"${lux_SOURCE_DIR}/../boost"			)
