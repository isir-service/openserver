project(interface VERSION 1.0)
message(${PROJECT_NAME})
aux_source_directory(. src_file)
add_library(interface SHARED ${src_file})