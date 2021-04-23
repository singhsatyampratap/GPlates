#
# Useful CMAKE variables.
#

# The GPlates package vendor.
set(GPLATES_PACKAGE_VENDOR "Earthbyte project")

# A short description of the GPlates project (only a few words).
set(GPLATES_PACKAGE_DESCRIPTION_SUMMARY "GPlates is desktop software for the interactive visualisation of plate tectonics.")

# The GPlates copyright - string version to be used in a source file.
set(GPLATES_COPYRIGHT_STRING "")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2003-2020 The University of Sydney, Australia\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2004-2020 California Institute of Technology\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2007-2020 The Geological Survey of Norway\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source code also contains code derived from:\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * ReconTreeViewer (James Boyden)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Boost intrusive_ptr (Peter Dimov)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki RefToValue (Richard Sposato, Peter Kummel)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.")

# The GPlates copyright for html.
set(GPLATES_HTML_COPYRIGHT_STRING "")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<html><body>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2003-2020 The University of Sydney, Australia<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2004-2020 California Institute of Technology<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2007-2020 The Geological Survey of Norway<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source code also contains code derived from: <ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> ReconTreeViewer (James Boyden) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Boost intrusive_ptr (Peter Dimov) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki RefToValue (Richard Sposato, Peter Kummel) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</body></html>\\n")

# Set to 'true' if this is a public code release (to non-developers).
# Currently disables all warnings.
# And also defines a compiler flag GPLATES_PUBLIC_RELEASE.
option(GPLATES_PUBLIC_RELEASE "Public release (to non-developers)." false)

# Whether to include sample data in the binary installer or not.
# By default this is false and only enabled when packaging a public release.
#
# Developers may want to turn this on, using the cmake command-line or cmake GUI, even when not releasing a public build.
option(GPLATES_INCLUDE_SAMPLE_DATA "Include sample data in the binary installer." false)

# The directory location of the sample data.
# The sample data is only included in the binary installer if 'GPLATES_INCLUDE_SAMPLE_DATA' is true.
# Paths must be full paths (eg, '~/sample-data' is ok but '../sample-data' is not).
set(GPLATES_SAMPLE_DATA_DIR "" CACHE PATH "Location of sample data.")

# The macOS code signing identity used to sign installed/packaged GPlates application bundle with a Developer ID certificate.
#
# NOTE: Leave it as the *empty* string here (so it doesn't get checked into source code control).
#       User is responsible to setting it to their ID (eg, using 'cmake -D', or CMake GUI, or 'ccmake').
#       It should typically be installed into the Keychain and look something like "Developer ID Application: <ID>".
if (APPLE)
	set(GPLATES_APPLE_CODE_SIGN_IDENTITY "" CACHE STRING "Apple code signing identity.")
endif()

# Set to 'true' to tell GPlates ignore environment variables, such as PYTHONHOME, PYTHONPATH, etc, 
# when initializing embeded Python interpreter.
# Put this close to GPLATES_PUBLIC_RELEASE because we usually want to set this to "true" when compiling public release.
#
# TODO: Consider adding a "python37._pth", for example, alongside the GPlates installed executable to ignore paths listed
# in the registry and environment variables (according to https://docs.python.org/3/using/windows.html#finding-modules).
set(GPLATES_IGNORE_PYTHON_ENVIRONMENT false)

# We compile with Python 3 (by default).
#
# However developers can choose to compile with Python 2 instead.
option(GPLATES_PYTHON_3 "Compile with Python 3 (not Python 2)." true)

# Whether to enable GPlates custom CPU profiling functionality.
#
# Is false by default. However note that the custom build type 'ProfileGplates' effectively
# overrides by adding 'GPLATES_PROFILE_CODE' directly as a compiler flag. Other build types
# set it indirectly via '#cmakedefine GPLATES_PROFILE_CODE' in 'config.h.in' using the
# same-named CMake variable (ie, the option below).
#
# Usually it's easiest to just select the 'ProfileGplates' build type (note that enabling/disabling
# the option below then has no effect). However, being a custom build type, that sometimes creates problems
# (eg, the CGAL dependency does not always play nicely with custom build types). In this case you can choose
# the builtin 'Release' build type (for example) and enable this option to achieve the same affect.
option(GPLATES_PROFILE_CODE "Enable GPlates custom CPU profiling functionality." false)

# Pre-compiled headers are turned off by default.
#
# Developers may want to turn this on using the cmake command-line or cmake GUI.
#
# Only CMake 3.16 and above support pre-compiled headers natively.
if (COMMAND target_precompile_headers)
	option(GPLATES_USE_PRECOMPILED_HEADERS "Use pre-compiled headers to speed up build times." false)
endif()

if (MSVC)
	# When using Visual Studio this shows included headers (used by 'list_external_includes.py').
	# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PRECOMPILED_HEADERS').
	#
	# Developers may want to turn this on using the cmake command-line or cmake GUI.
	option(GPLATES_MSVC_SHOW_INCLUDES "Show included headers. Used when configuring pre-compiled headers." false)
	mark_as_advanced(GPLATES_MSVC_SHOW_INCLUDES)
	# Disable pre-compiled headers if showing include headers.
	# The only reason to show include headers is to use 'list_external_includes.py' script to generates pch header.
	if (GPLATES_MSVC_SHOW_INCLUDES)
		# Note: This sets the non-cache variable ('option' above sets the cache variable of same name).
		#       The non-cache variable will get precedence when subsequently accessed.
		#       It's also important to set this *after* 'option' since whenever a cache variable is added
		#       (eg, on the first run if not yet present in "CMakeCache.txt") the normal variable is removed.
		if (DEFINED GPLATES_USE_PRECOMPILED_HEADERS)
			set(GPLATES_USE_PRECOMPILED_HEADERS false)
		endif()
	endif()

	# If Visual Studio then enable parallel builds within a project.
	#
	# To also enable parallel project builds set
	# Tools->Options->Programs and Solutions->Build and Run->maximum number of parallel project builds to
	# the number of cores on your CPU.
	#
	# This is on by default otherwise compilation will take a long time.
	option(GPLATES_MSVC_PARALLEL_BUILD "Enable parallel builds within each Visual Studio project." true)
endif()

# Specify which source directories (relative to the 'doc/' directory) should be scanned by doxygen.
set(GPLATES_DOXYGEN_INPUT
    "../src/feature-visitors ../src/file-io ../src/model ../src/property-values ../src/utils")


# The location of the GPlates executable is placed here when it is built (but not installed).
# Note that this is different from the "RUNTIME DESTINATION" in the "install" command which specifies
# the suffix path of where the installed executable goes (versus the built executable).
#
# Set default location of built (not installed) executables on all platforms and DLLs on Windows.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${GPlates_BINARY_DIR}/bin")
# Set default location for built (not installed) shared libraries on non-Windows platforms.
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${GPlates_BINARY_DIR}/bin")

# Set the minimum C++ language standard to C++11.
#
# std::auto_ptr was deprecated in C++11 (and removed in C++17), so we now use std::unique_ptr introduced in C++11.
# Also GDAL 2.3 and above require C++11.
# And CGAL 5.x requires C++14, and this requirement is transitively passed to us via the CGAL::CGAL target
# where CMake chooses the stricter requirement between our C++11 and CGAL's C++14 (which is C++14).
#
# Also note that this avoids the problem of compile errors, when C++14 features are used (eg, by CGAL 5),
# due to forcing C++11 by specifying '-std=c++11' directly on the compiler command-line.
#
# Note: This is also set using 'target_compile_features()' in src/CMakeLists.txt (for CMake >= 3.8).
#       So this can be removed once our minimum CMake requirement is 3.8.
if (CMAKE_VERSION VERSION_LESS 3.8)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Disable compiler-specific extensions.
# Eg, for C++11 on GNU compilers we want '-std=c++11' instead of '-std=g++11'.
set(CMAKE_CXX_EXTENSIONS OFF)

# Order the include directories so that directories which are in the source or build tree always
# come before directories outside the project.
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE true)
