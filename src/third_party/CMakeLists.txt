set(CMAKE_UNITY_BUILD ON)

set(TITANENGINE_RESOURCES OFF CACHE BOOL "" FORCE)
add_subdirectory(TitanEngine)

set(GLEEBUG_RESOURCES OFF CACHE BOOL "" FORCE)
add_subdirectory(GleeBug)
