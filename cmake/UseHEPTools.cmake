
include(${CMAKE_CURRENT_LIST_DIR}/HEPToolsMacros.cmake)

# use_heptools(heptools_version)
#
# Look for the required version of the HEPTools toolchain and replace the
# current toolchain file with it.
#
# WARNING: this macro must be called from a toolchain file
#
macro(use_heptools heptools_version)
  # Deduce the LCG configuration tag from the system
  lcg_detect_host_platform()
  lcg_get_target_platform()

  # Find the toolchain description
  find_file(LCG_TOOLCHAIN_INFO
            NAMES LCG_${heptools_version}_${BINARY_TAG}.txt
                  LCG_${heptools_version}_${LCG_platform}.txt
                  LCG_${heptools_version}_${LCG_system}.txt
                  LCG_externals_${BINARY_TAG}.txt
                  LCG_externals_${LCG_platform}.txt
                  LCG_externals_${LCG_system}.txt
            HINTS ENV CMTPROJECTPATH
            PATH_SUFFIXES LCG_${heptools_version})

  if(LCG_TOOLCHAIN_INFO)
    message(STATUS "Using heptools ${heptools_version} from ${LCG_TOOLCHAIN_INFO}")

    get_filename_component(LCG_releases ${LCG_TOOLCHAIN_INFO} PATH CACHE)
    set(LCG_external ${LCG_releases})
    if(LCG_releases MATCHES "LCG_${heptools_version}\$")
      get_filename_component(LCG_releases_base ${LCG_releases} PATH)
    else()
      set(LCG_releases_base ${LCG_releases})
    endif()

    file(STRINGS ${LCG_TOOLCHAIN_INFO} _lcg_infos)

    if(LCG_FORCE_COMPILER)
      string(REPLACE "-" ";" _l "${LCG_FORCE_COMPILER}")
      lcg_set_compiler(${_l})
    endif()

    foreach(_l ${_lcg_infos})
      if(_l MATCHES "^COMPILER:")
        string(REPLACE "COMPILER:" "" _l "${_l}")
        string(STRIP _l "${_l}")
        string(REGEX REPLACE " +" ";" _l "${_l}")
        if (NOT LCG_FORCE_COMPILER)
          lcg_set_compiler(${_l})
        endif()
        # We add the compiler to the path
        lcg_set_lcg_system_compiler_path(${_l})
      else()
         if(NOT _l MATCHES "^((PLATFORM|VERSION|COMPILER):|#)")
           string(REGEX REPLACE "; *" ";" _l "${_l}")
           lcg_set_external(${_l})
         endif()
      endif()
    endforeach()


    # Enable the right compiler (needs LCG_external)
    lcg_common_compilers_definitions()

    lcg_prepare_paths()

    lcg_find_common_tools()

    # Reset the cache variable to have proper documentation.
    #set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST}
    #    CACHE FILEPATH "The CMake toolchain file" FORCE)

  else()

    # try old toolchain style

    # Remove the reference to this file from the cache.
    unset(CMAKE_TOOLCHAIN_FILE CACHE)

    # Find the actual toolchain file.
    find_file(CMAKE_TOOLCHAIN_FILE
              NAMES heptools-${heptools_version}.cmake
              HINTS ENV CMTPROJECTPATH
              PATHS ${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain
              PATH_SUFFIXES toolchain)

    if(NOT CMAKE_TOOLCHAIN_FILE)
      message(FATAL_ERROR "Cannot find heptools ${heptools_version}.")
    endif()

    # Reset the cache variable to have proper documentation.
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE}
        CACHE FILEPATH "The CMake toolchain file" FORCE)

    message(STATUS "Using heptools version ${heptools_version}")
    include(${CMAKE_TOOLCHAIN_FILE})

  endif()

endmacro()
