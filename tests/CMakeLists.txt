# Testing library
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Tests need to be added as executables first
add_executable(CallbackTest CallbackTest.cpp)
add_executable(StrandTest StrandTest.cpp)

# Should be linked to the main library, as well as the testing library
target_link_libraries(CallbackTest PRIVATE GTest::gtest_main)
target_link_libraries(StrandTest PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(CallbackTest)
gtest_discover_tests(StrandTest)