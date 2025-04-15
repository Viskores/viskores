##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

# Read the files from the build directory that contain
# host information ( name, parallel level, etc )
include("$ENV{CI_PROJECT_DIR}/build/CIState.cmake")
ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")

set(ccache_stat_logs "${CTEST_BINARY_DIRECTORY}/ccache_stats.txt")
set(CTEST_NOTES_FILES "${ccache_stat_logs}")
execute_process(COMMAND ccache -s ERROR_FILE "${ccache_stat_logs}" OUTPUT_FILE "${ccache_stat_logs}")

# Pick up from where the configure left off.
ctest_start(APPEND)

# Submit notes from the build step
ctest_submit(PARTS Notes)
