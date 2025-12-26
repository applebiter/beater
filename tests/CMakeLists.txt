# Create separate test executables
add_executable(test_timeutils test_TimeUtils.cpp)
target_link_libraries(test_timeutils PRIVATE beater_domain)
add_test(NAME TimeUtilsTest COMMAND test_timeutils)

add_executable(test_pattern test_Pattern.cpp)
target_link_libraries(test_pattern PRIVATE beater_domain)
add_test(NAME PatternTest COMMAND test_pattern)

add_executable(test_track test_Track.cpp)
target_link_libraries(test_track PRIVATE beater_domain)
add_test(NAME TrackTest COMMAND test_track)
