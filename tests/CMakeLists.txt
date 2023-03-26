find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_executable(CorrectnessTests correctness.cpp)

target_link_libraries(CorrectnessTests PRIVATE unofficial::sqlite3::sqlite3 GTest::gtest GTest::gtest_main)

add_test(CorrectnessTests CorrectnessTests)