#
# Useful CMAKE variables.
#
# There are three configuration files:
#   1) "ConfigDefault.cmake" - is version controlled and used to add new default variables and set defaults for everyone.
#   2) "ConfigUser.cmake" in the source tree - is not version controlled (currently listed in svn:ignore property) and used to override defaults on a per-user basis.
#   3) "ConfigUser.cmake" in the build tree - is used to override "ConfigUser.cmake" in the source tree.
#
# NOTE: If you want to change CMake behaviour just for yourself then modify the "ConfigUser.cmake" file (not "ConfigDefault.cmake").
#
include ("${CMAKE_SOURCE_DIR}/cmake/ConfigDefault.cmake")

# If "ConfigUser.cmake" doesn't exist then create one for convenience.
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")
    file(WRITE "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake"
        "# Use this file to override variables in 'ConfigDefault.cmake' on a per-user basis.\n"
        "# This file is not version controlled.\n"
        "\n"
        "# Pre-compiled headers are turned off by default.\n"
        "# Developers of GPlates may want to turn them on here to speed up build times.\n"
        "set(GPLATES_USE_PCH false)\n\n"
        "# When using Visual Studio this shows included headers (used by 'list_external_includes.py').\n"
        "# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PCH').\n"
        "set(GPLATES_SHOW_INCLUDES false)\n\n"
        "# If Visual Studio 2005 then enable parallel builds within a project.\n"
        "#\n"
        "# To also enable parallel project builds set\n"
        "# �Tools->Options->Programs and Solutions->Build and Run->maximum number of parallel project builds� to\n"
        "# the number of cores on your CPU.\n"
        "set(GPLATES_MSVC80_PARALLEL_BUILD true)\n\n")
endif (NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")
include ("${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")

# If you've got a 'ConfigUser.cmake' in the build tree then that overrides the
# one in the source tree.
if (EXISTS "${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")
    include ("${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")
endif (EXISTS "${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")

###########################################################
# Do any needed processing of the configuration variables #
###########################################################

# Convert GPLATES_BINARY_INSTALL_EXTRAS to absolute paths.
# Should already be full paths (eg, this can convert
# "~/sample-data" to "/Users/gplates/sample-data").
# At same time as converting to absolute path also convert from a
# list variable (a string with semicolon-separated entries)
# to a string with spaced-separated entries.
set(GPLATES_BINARY_INSTALL_EXTRAS_TMP ${GPLATES_BINARY_INSTALL_EXTRAS})
set(GPLATES_BINARY_INSTALL_EXTRAS "")
foreach(extra ${GPLATES_BINARY_INSTALL_EXTRAS_TMP})
	get_filename_component(extra "${extra}" ABSOLUTE)
	set(GPLATES_BINARY_INSTALL_EXTRAS "${GPLATES_BINARY_INSTALL_EXTRAS} ${extra}")
endforeach(extra ${GPLATES_BINARY_INSTALL_EXTRAS_TMP})
