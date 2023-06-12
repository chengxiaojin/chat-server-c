#把 . 指定路径下的所有源文件名字放入变量名SRC_LIST里面
aux_source_directory(. SRC_LIST)

#表示生成的可执行文件  由变量名SRC_LIST变量所定义的源文件编译而来
add_executable(ChatClient ${SRC_LIST} )

#表示client这个目标程序 需要链接的库文件
#因为有两个线程  读线程 == 处理用户发来的数据  写线程 == 给服务器端发送数据
target_link_libraries(ChatClient pthread)


#配置头文件搜索路径
# include_directories(${PROJECT_SOURCE_DIR}/include)
# include_directories(${PROJECT_SOURCE_DIR}/include/server)
# include_directories(${PROJECT_SOURCE_DIR}/thirdparty)