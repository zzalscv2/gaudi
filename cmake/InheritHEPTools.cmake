# Special toolchain file that looks for the projects used and choose the version
# of the heptools toolchain from them.

cmake_minimum_required(VERSION 2.8.5)

# detect the required heptols version
function(guess_heptools_version var)

  if(NOT projects)
    include(GaudiToolchainMacros)
    init()
    find_projects(projects tools ${CMAKE_SOURCE_DIR}/CMakeLists.txt)
  endif()

  foreach(_project ${projects})
    file(GLOB configs ${${_project}_ROOT_DIR}/InstallArea/*/*Config.cmake)
    foreach(config ${configs})
      file(READ ${config} config_file)
      if(config_file MATCHES "set *\\( *[^ ]*_heptools_version *([^ ]*) *\\)")
        set(${var} ${CMAKE_MATCH_1} PARENT_SCOPE)
        message(STATUS "Detected heptools version ${CMAKE_MATCH_1} in ${${_project}_ROOT_DIR}")
        return()
      endif()
    endforeach()
  endforeach()

endfunction()

include(UseHEPTools)

macro(inherit_heptools)
  guess_heptools_version(heptools_version)
  if(heptools_version)
    use_heptools(${heptools_version})
  endif()
endmacro()
