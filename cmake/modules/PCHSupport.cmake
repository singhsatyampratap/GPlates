# - Try to find precompiled headers support for GCC 3.4 and 4.x
# Once done this will define:
#
# Variable:
#   PCHSupport_FOUND
#
# Macro:
#   ADD_PRECOMPILED_HEADER  _targetName _input  _dowarn
#
#   ADD_NATIVE_PRECOMPILED_HEADER _targetName _input _dowarn
#   GET_NATIVE_PRECOMPILED_HEADER _targetName _input

# This file was found at http://www.cmake.org/Bug/view.php?id=1260

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

IF(CMAKE_COMPILER_IS_GNUCXX)
    # Verifies if GCC supports precompiled header
    # Its version should be >= 3.4.0
    EXEC_PROGRAM(
    	${CMAKE_CXX_COMPILER}  
        ARGS 	${CMAKE_CXX_COMPILER_ARG1} -dumpversion 
        OUTPUT_VARIABLE gcc_compiler_version)
	#MESSAGE("GCC Version: ${gcc_compiler_version}")
    IF(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
        SET(PCHSupport_FOUND TRUE)
    ELSE()
        IF(gcc_compiler_version MATCHES "3\\.4\\.[0-9]")
            SET(PCHSupport_FOUND TRUE)
        ENDIF()
    ENDIF()
    
    # Use '-isystem' instead of '-I' since our pre-compiled
    # headers happen to all be external to GPlates and so
    # we can be lenient on warnings generated by them.
    # '-isystem' gives this leniency (is currently needed for GDAL library).
    # FIXME: this will reduce warnings for GPlates source code if we
    # ever include GPlates internal headers to our precompiled headers.
    if(SYSTEM_INCLUDE_FLAG STREQUAL "SYSTEM")
        SET(_PCH_include_prefix -isystem)
    else(SYSTEM_INCLUDE_FLAG STREQUAL "SYSTEM")
        SET(_PCH_include_prefix -I)
    endif(SYSTEM_INCLUDE_FLAG STREQUAL "SYSTEM")
    message("_PCH_include_prefix=${_PCH_include_prefix}")
ELSE()
    IF(WIN32)	
        SET(PCHSupport_FOUND TRUE) # for experimental msvc support
        SET(_PCH_include_prefix /I)
    ELSE()
        SET(PCHSupport_FOUND FALSE)
    ENDIF()	
ENDIF()


MACRO(_PCH_GET_COMPILE_FLAGS _out_compile_flags)
    SET(${_out_compile_flags} ${CMAKE_CXX_FLAGS} )

    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    if(${_flags_var_name})
        list(APPEND ${_out_compile_flags} ${${_flags_var_name}} )
    endif(${_flags_var_name})

    # If the compiler is g++ and the target type is shared library, we have
    # to add -fPIC to its compile flags.
    IF(CMAKE_COMPILER_IS_GNUCXX)
        GET_TARGET_PROPERTY(_targetType ${_PCH_current_target} TYPE)
        IF(${_targetType} STREQUAL SHARED_LIBRARY)
            LIST(APPEND ${_out_compile_flags} -fPIC)
        ENDIF()
    ELSE(MSVC)
        LIST(APPEND ${_out_compile_flags} /Fd${CMAKE_CURRENT_BINARY_DIR}/${_PCH_current_target}.pdb)
    ENDIF()

    # Add all include directives...
    GET_DIRECTORY_PROPERTY(DIRINC INCLUDE_DIRECTORIES )
    FOREACH(item ${DIRINC})
        LIST(APPEND ${_out_compile_flags} ${_PCH_include_prefix}${item})
    ENDFOREACH(item)

    # Add all definitions...
    GET_TARGET_PROPERTY(_compiler_flags ${_PCH_current_target} COMPILE_FLAGS)
	if(_compiler_flags)
		LIST(APPEND ${_out_compile_flags} ${_compiler_flags})
	endif(_compiler_flags)

    GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	if(_directory_flags)
        LIST(APPEND ${_out_compile_flags} ${_directory_flags})
	endif(_directory_flags)

    get_target_property(_target_flags ${_PCH_current_target} DEFINITIONS)
	if(_target_flags)
        LIST(APPEND ${_out_compile_flags} ${_target_flags})
	endif(_target_flags)

    GET_DIRECTORY_PROPERTY(_directory_flags COMPILE_DEFINITIONS)
	foreach(_flag ${_directory_flags})
		if(_flag)
			LIST(APPEND ${_out_compile_flags} -D${_flag})
		endif(_flag)
	endforeach(_flag ${_directory_flags})

    get_target_property(_target_flags ${_PCH_current_target} COMPILE_DEFINITIONS)
	foreach(_flag ${_target_flags})
		if(_flag)
			LIST(APPEND ${_out_compile_flags} -D${_flag})
		endif(_flag)
	endforeach(_flag ${_target_flags})

    STRING(TOUPPER "DEFINITIONS_${CMAKE_BUILD_TYPE}" _defs_prop_name)
    GET_DIRECTORY_PROPERTY(_directory_flags ${_defs_prop_name})
	if(_directory_flags)
        LIST(APPEND ${_out_compile_flags} ${_directory_flags})
	endif(_directory_flags)

    get_target_property(_target_flags ${_PCH_current_target} ${_defs_prop_name})
	if(_target_flags)
        LIST(APPEND ${_out_compile_flags} ${_target_flags})
	endif(_target_flags)

    STRING(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" _defs_prop_name)
    GET_DIRECTORY_PROPERTY(_directory_flags ${_defs_prop_name})
	foreach(_flag ${_directory_flags})
		if(_flag)
			LIST(APPEND ${_out_compile_flags} -D${_flag})
		endif(_flag)
	endforeach(_flag ${_directory_flags})

    get_target_property(_target_flags ${_PCH_current_target} ${_defs_prop_name})
	foreach(_flag ${_target_flags})
		if(_flag)
			LIST(APPEND ${_out_compile_flags} -D${_flag})
		endif(_flag)
	endforeach(_flag ${_target_flags})

    SEPARATE_ARGUMENTS(${_out_compile_flags})
ENDMACRO(_PCH_GET_COMPILE_FLAGS)


MACRO(_PCH_GET_COMPILE_COMMAND out_command _input _output)
	FILE(TO_NATIVE_PATH ${_input} _native_input)
	FILE(TO_NATIVE_PATH ${_output} _native_output)

	IF(CMAKE_COMPILER_IS_GNUCXX)
        IF(CMAKE_CXX_COMPILER_ARG1)
	        # remove leading space in compiler argument
            STRING(REGEX REPLACE "^ +" "" pchsupport_compiler_cxx_arg1 ${CMAKE_CXX_COMPILER_ARG1})
        ELSE()
            SET(pchsupport_compiler_cxx_arg1 "")
        ENDIF()
        SET(${out_command} 
            ${CMAKE_CXX_COMPILER} ${pchsupport_compiler_cxx_arg1} ${_compile_FLAGS}	-x c++-header -o ${_output} ${_input})
    ELSE()
        set(_tempdir ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_PCH_current_target}.dir)
        GET_FILENAME_COMPONENT(_namewe ${_input} NAME_WE)
        SET(_pchsource ${_tempdir}/${_namewe}.cpp)
        FILE(TO_NATIVE_PATH ${_pchsource} _native_pchsource)

		SET(_dummy_str "#include \"${_input}\"")
		FILE(WRITE ${_pchsource} ${_dummy_str})
	
		SET(${out_command} 
			${CMAKE_CXX_COMPILER} ${_compile_FLAGS} /c /Fp${_native_output} /Yc${_native_input} /Fo${_tempdir}/ ${_native_pchsource} /nologo)	
	ENDIF()
ENDMACRO(_PCH_GET_COMPILE_COMMAND )

MACRO(_PCH_GET_TARGET_COMPILE_FLAGS _cflags  _header_name _pch_path _dowarn )
    FILE(TO_NATIVE_PATH ${_pch_path} _native_pch_path)
#message(${_native_pch_path})

    IF(CMAKE_COMPILER_IS_GNUCXX)
        # for use with distcc and gcc>4.0.1 if preprocessed files are accessible
        # on all remote machines set
        # PCH_ADDITIONAL_COMPILER_FLAGS to -fpch-preprocess
        # if you want warnings for invalid header files (which is very 
        # inconvenient if you have different versions of the headers for
        # different build types you may set _pch_dowarn
        LIST(APPEND ${_cflags} -include ${CMAKE_CURRENT_BINARY_DIR}/${_header_name})
        IF(${PCH_ADDITIONAL_COMPILER_FLAGS})
            LIST(APPEND ${_cflags} ${PCH_ADDITIONAL_COMPILER_FLAGS})
        ENDIF(${PCH_ADDITIONAL_COMPILER_FLAGS})
		IF (_dowarn)
            LIST(APPEND ${_cflags} -Winvalid-pch)
		ENDIF ()
    ELSE()
        set(${_cflags} "/Fp${_native_pch_path}" "/Yu${_header_name}" )	
    ENDIF()	

	get_target_property(_old_target_cflags ${_PCH_current_target} COMPILE_FLAGS)
	if(_old_target_cflags)
		list(APPEND ${_cflags} ${_old_target_cflags})
	endif()

    STRING(REPLACE ";" " " ${_cflags} "${${_cflags}}")
ENDMACRO(_PCH_GET_TARGET_COMPILE_FLAGS )

MACRO(GET_PRECOMPILED_HEADER_OUTPUT _targetName _input _output)
    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(_build "")
        IF(CMAKE_BUILD_TYPE)
            set(_build _${CMAKE_BUILD_TYPE})
        endif()
		file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch)
        SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch/${_targetName}${_build}.h++")
    else()
        SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${_name}.pch")
    endif()
ENDMACRO(GET_PRECOMPILED_HEADER_OUTPUT _targetName _input)

MACRO(ADD_PRECOMPILED_HEADER _targetName _input _targetSources)
    SET(_PCH_current_target ${_targetName})

	if(${ARGN})
		set(_dowarn 1)
	else()
		set(_dowarn 0)
	endif()

    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    GET_FILENAME_COMPONENT(_path ${_input} PATH)
    GET_PRECOMPILED_HEADER_OUTPUT( ${_targetName} ${_input} _output)
    GET_FILENAME_COMPONENT(_outdir ${_output} PATH )

    FILE(MAKE_DIRECTORY ${_outdir})

    _PCH_GET_COMPILE_FLAGS(_compile_FLAGS)

    #Ensure same directory! Required by gcc
    IF(NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
        SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/${_name} PROPERTIES GENERATED 1)
        ADD_CUSTOM_COMMAND(OUTPUT	${CMAKE_CURRENT_BINARY_DIR}/${_name} 
                           COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_CURRENT_SOURCE_DIR}/${_input} ${CMAKE_CURRENT_BINARY_DIR}/${_name}
                           DEPENDS ${_input})
    ENDIF()

    _PCH_GET_COMPILE_COMMAND(_command  ${CMAKE_CURRENT_BINARY_DIR}/${_name} ${_output} )
    #message("${_command}")

    ADD_CUSTOM_COMMAND(OUTPUT ${_output}
                       COMMAND ${_command}
                       DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_name})
                       #IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_BINARY_DIR}/${_name})


    #message("_targetSources=${${_targetSources}}")
    # GET_TARGET_PROPERTY(_sources ${_targetName} SOURCES)
    iF(${_targetSources})
        FOREACH(_src ${${_targetSources}})
            SET_SOURCE_FILES_PROPERTIES(${_src} PROPERTIES OBJECT_DEPENDS ${_output})
        ENDFOREACH()
    ENDIF(${_targetSources})

    # Set '_target_cflags' to empty because _PCH_GET_TARGET_COMPILE_FLAGS
    # adds to it and would otherwise accumulate over multiple targets.
    set(_target_cflags )
    _PCH_GET_TARGET_COMPILE_FLAGS(_target_cflags ${_name} ${_output} ${_dowarn})

	if(_target_cflags)
		SET_TARGET_PROPERTIES(${_targetName} 
							  PROPERTIES	
							  COMPILE_FLAGS ${_target_cflags} )
	endif(_target_cflags)
ENDMACRO(ADD_PRECOMPILED_HEADER)


# Generates the use of precompiled in a target,
# without using depency targets (2 extra for each target)
# Using Visual, must also add ${_targetName}_pch to sources
# Not needed by Xcode

MACRO(GET_NATIVE_PRECOMPILED_HEADER _targetName _input)

	if(CMAKE_GENERATOR MATCHES Visual*)

		STRING(REPLACE "-" "_" _targetNameCToken ${_targetName})
		SET(_dummy_str "#include \"${_input}\"\n"
										"// This is required to suppress LNK4221.  Very annoying.\n"
										"void *g_${_targetNameCToken}Dummy = 0\;\n")

		# Use of cc extension for generated files.
		SET(${_targetName}_pch ${CMAKE_CURRENT_BINARY_DIR}/${_targetName}_pch.cc)
		if(EXISTS ${${_targetName}_pch})
			# Check if contents is the same, if not rewrite
			# todo
		else(EXISTS ${${_targetName}_pch})
			FILE(WRITE ${${_targetName}_pch} ${_dummy_str})
		endif(EXISTS ${${_targetName}_pch})
	endif(CMAKE_GENERATOR MATCHES Visual*)

ENDMACRO(GET_NATIVE_PRECOMPILED_HEADER)


MACRO(ADD_NATIVE_PRECOMPILED_HEADER _targetName _input)

	IF( "${ARGN}" STREQUAL "0")
		SET(_dowarn 0)
	ELSE( "${ARGN}" STREQUAL "0")
		SET(_dowarn 1)
	ENDIF("${ARGN}" STREQUAL "0")
	
	if(CMAKE_GENERATOR MATCHES Visual*)
		# Auto include the precompile (useful for moc processing, since the use of 
		# precompiled is specified at the target level
		# and I don't want to specifiy /F- for each moc/res/ui generated files (using Qt)

		GET_TARGET_PROPERTY(oldProps ${_targetName} COMPILE_FLAGS)
		if (${oldProps} MATCHES NOTFOUND)
			SET(oldProps "")
		endif(${oldProps} MATCHES NOTFOUND)

		SET(newProperties "${oldProps} /Yu\"${_input}\" /FI\"${_input}\"")
		SET_TARGET_PROPERTIES(${_targetName} PROPERTIES COMPILE_FLAGS "${newProperties}")
		
		#also inlude ${oldProps} to have the same compile options 
		SET_SOURCE_FILES_PROPERTIES(${${_targetName}_pch} PROPERTIES COMPILE_FLAGS "${oldProps} /Yc\"${_input}\"")
		
	else(CMAKE_GENERATOR MATCHES Visual*)
	
		if (CMAKE_GENERATOR MATCHES Xcode)
			# For Xcode, cmake needs my patch to process
			# GCC_PREFIX_HEADER and GCC_PRECOMPILE_PREFIX_HEADER as target properties
			
			GET_TARGET_PROPERTY(oldProps ${_targetName} COMPILE_FLAGS)
			if (${oldProps} MATCHES NOTFOUND)
				SET(oldProps "")
			endif(${oldProps} MATCHES NOTFOUND)

			# When buiding out of the tree, precompiled may not be located
			# Use full path instead.
			GET_FILENAME_COMPONENT(fullPath ${_input} ABSOLUTE)

			SET_TARGET_PROPERTIES(${_targetName} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${fullPath}")
			SET_TARGET_PROPERTIES(${_targetName} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")

		else (CMAKE_GENERATOR MATCHES Xcode)

			#Fallback to the "old" precompiled suppport
			ADD_PRECOMPILED_HEADER(${_targetName} ${_input} ${_dowarn})
		endif(CMAKE_GENERATOR MATCHES Xcode)
	endif(CMAKE_GENERATOR MATCHES Visual*)

ENDMACRO(ADD_NATIVE_PRECOMPILED_HEADER)
