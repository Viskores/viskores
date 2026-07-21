add_library(viskores_loguru INTERFACE)

viskores_get_kit_name(kit_name kit_dir)

# loguru needs C++11
target_compile_features(viskores_loguru INTERFACE cxx_std_11)

target_include_directories(viskores_loguru INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_link_libraries(viskores_loguru INTERFACE ${CMAKE_DL_LIBS} Threads::Threads)

viskores_install_targets(TARGETS viskores_loguru)
