# SPDX-License-Identifier: GPL-3.0-or-later
# ESP-IDF component. The simulator does not read this file: it globs src/*.cpp
# and adds include/ itself, so one source tree serves both platforms.
file(GLOB SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "include"
    REQUIRES crosspad-core crosspad-gui lvgl
)

# Keeps the REGISTER_APP-generated _register_<Name>_app() alive through linking.
set_property(TARGET ${COMPONENT_LIB} PROPERTY WHOLE_ARCHIVE ON)
