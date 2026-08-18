# Characterization tests for the mod's pure logic. Deliberately links nothing
# from the game or the hooks: anything that needs a live client.dll is verified
# in-game (see .lab/NOTES.md), and anything that does not belongs here.
add_executable(portal2_headtracking_tests
    test_main.cpp
    source_math_tests.cpp
    position_mapping_tests.cpp
    reticle_projection_tests.cpp
    ${CMAKE_SOURCE_DIR}/src/source_math.cpp
)

# The core include dir is for header-only types only (PositionSettings), so the
# suite still links nothing from the game or the hooks.
target_include_directories(portal2_headtracking_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/cameraunlock-core/cpp/include
)

if(MSVC)
    target_compile_options(portal2_headtracking_tests PRIVATE /W4 /WX)
else()
    target_compile_options(portal2_headtracking_tests PRIVATE -Wall -Wextra -Werror)
endif()

add_test(NAME portal2_headtracking_tests COMMAND portal2_headtracking_tests)
