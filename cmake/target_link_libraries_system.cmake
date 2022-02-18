function(target_link_libraries_system target scope)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} SYSTEM ${scope} ${lib_include_dirs})
    target_link_libraries(${target} ${scope} ${lib})
  endforeach(lib)
endfunction(target_link_libraries_system)

