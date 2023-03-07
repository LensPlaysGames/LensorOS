execute_process(COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/mkfat32efibootimage.cmake)
execute_process(COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/mkfat32lensorosdata.cmake)
