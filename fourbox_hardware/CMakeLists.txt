cmake_minimum_required(VERSION 3.8)
project(fourbox_hardware)

find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)

# ✅ 1. Include директория ПЕРЕД библиотекой
add_library(fourbox_hardware SHARED 
  src/fourbox_hardware.cpp
)

# ✅ 2. Headers (ваш стиль)
target_include_directories(fourbox_hardware
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    src
)

target_compile_features(fourbox_hardware PRIVATE cxx_std_17)

target_link_libraries(fourbox_hardware
  hardware_interface::hardware_interface
  pluginlib::pluginlib     # ✅ Добавлено!
  rclcpp::rclcpp
)

# ✅ Plugin XML
pluginlib_export_plugin_description_file(hardware_interface fourbox_hardware.xml)

# ✅ Установка config (если есть)
install(
  DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}/
)

install(TARGETS fourbox_hardware
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION lib/${PROJECT_NAME}/include
  FILES_MATCHING PATTERN "*.hpp"
)

install(FILES fourbox_hardware.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(fourbox_hardware)
ament_export_dependencies(
  hardware_interface 
  pluginlib 
  rclcpp
)
ament_package()
