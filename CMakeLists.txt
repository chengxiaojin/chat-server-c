cmake_minimum_required(VERSION 3.0)  #最小版本

project(chat)

#存储可执行文件的最终存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#指定搜索的子目录  如果放在最外的根目录下的话
#add_subdirectory(testmuduo)

#配置吧编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
#加载子目录
add_subdirectory(src)

#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
#set(SRC_LIST ./muduo_server.cc)

#把 . 指定路径下的所有源文件名字放入变量名SRC_LIST里面
#aux_source_directory(. SRC_LIST)

#表示生成的可执行文件  由变量名SRC_LIST变量所定义的源文件编译而来
#add_executable(server ${SRC_LIST})

#表示server这个目标程序 需要链接的库文件
#target_link_libraries(server muduo_net muduo_base pthread)


 