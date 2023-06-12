#把 . 指定路径下的所有源文件名字放入变量名SRC_LIST里面
aux_source_directory(. SRC_LIST)

aux_source_directory(./db DB_LIST)

aux_source_directory(./model MODEL_LIST)

aux_source_directory(./redis REDIS_LIST)
#表示生成的可执行文件  由变量名SRC_LIST变量所定义的源文件编译而来
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#表示server这个目标程序 需要链接的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)


#配置头文件搜索路径
# include_directories(${PROJECT_SOURCE_DIR}/include)
# include_directories(${PROJECT_SOURCE_DIR}/include/server)
# include_directories(${PROJECT_SOURCE_DIR}/thirdparty)