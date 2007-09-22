PROJECT(lux)
OPTION(WITH_GUI "Enable the GUI." OFF)
OPTION(WITH_SSE "Enable SSE optimisations" ON)

# Find and setup OpenEXR
#FIND_LIBRARY(OpenEXR REQUIRED)
  FIND_PATH(OPENEXR_INC
    ImfXdr.h
    PATHS
    /usr/local/include/OpenEXR
    /usr/include/OpenEXR
    /sw/include/OpenEXR
    /opt/local/include/OpenEXR
    /opt/csw/include/OpenEXR
    /opt/include/OpenEXR
  )
  SET(OPENEXR_LIB Half IlmImf Iex Imath)

FIND_PROGRAM(BISON_EXECUTABLE bison)
IF (NOT BISON_EXECUTABLE)
MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_EXECUTABLE)

FIND_PROGRAM(FLEX_EXECUTABLE flex)
IF (NOT FLEX_EXECUTABLE)
MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_EXECUTABLE)


ADD_SUBDIRECTORY(core)