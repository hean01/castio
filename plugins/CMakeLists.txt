find_program(ZIP_EXECUTABLE 7z)
if (NOT ZIP_EXECUTABLE)
  find_program(ZIP_EXECUTABLE 7za)
endif (NOT ZIP_EXECUTABLE)

if (NOT ZIP_EXECUTABLE)
  message(FATAL_ERROR "p7zip is required to build plugin bundles.")
endif (NOT ZIP_EXECUTABLE)

#
# plugin macro to create zip bundle
#
macro (plugin _name)
  set(_output ${CMAKE_CURRENT_BINARY_DIR}/${_name}.zip)
  file(GLOB _plugin_file_list ${CMAKE_CURRENT_SOURCE_DIR}/${_name}/*)
  set(_plugin_files "")
  foreach(arg ${_plugin_file_list})
    set(_plugin_files "${_plugin_files}" "${arg}")
  endforeach()

  add_custom_command(
    OUTPUT "${_output}"
    COMMAND ${ZIP_EXECUTABLE} a -tzip ${_output} ${_plugin_files} > /dev/null)

  add_custom_target(${_name}.zip ALL DEPENDS ${_output})
  set_target_properties(${_name}.zip PROPERTIES OUTPUT_NAME "${_name}.zip")
  install(FILES ${_output} DESTINATION ${CASTIO_DATA_DIR}/plugins)

endmacro (plugin)

plugin("sample")
plugin("svtplay")
plugin("icecast")
plugin("hvsc")
plugin("kwed")
plugin("di")