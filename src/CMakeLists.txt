# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources})

# Monochrome
lvgl_port_create_c_image("images/teslasynth_tiny.png" "images/" "I1" "NONE")

# Icons 16px
lvgl_port_create_c_image("images/play_icon_16px.png" "images/" "A1" "NONE")
lvgl_port_create_c_image("images/pause_icon_16px.png" "images/" "A1" "NONE")
lvgl_port_create_c_image("images/bluetooth_icon_16px.png" "images/" "A1" "NONE")

# Colored
lvgl_port_create_c_image("images/teslasynth_240p_large.png" "images/" "I4" "NONE")
lvgl_port_create_c_image("images/teslasynth_240p_small.png" "images/" "I4" "NONE")
lvgl_port_add_images(${COMPONENT_LIB} "images/")
