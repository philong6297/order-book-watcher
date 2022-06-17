project(project_test)

add_executable(project_test)
target_compile_options(project_test PRIVATE ${LONGLP_DESIRED_COMPILE_OPTIONS})
target_compile_features(project_test PRIVATE ${LONGLP_DESIRED_COMPILE_FEATURES})
target_include_directories(project_test PRIVATE ${LONGLP_PROJECT_SRC_DIR})
target_link_libraries(
  project_test
  PRIVATE GTest::gtest nlohmann_json::nlohmann_json fmt::fmt Taskflow::Taskflow
          Threads::Threads
          # target for each solution
)
target_sources(
  project_test
  PRIVATE main.cpp
          # unittest for each solution
          instrument_feeds_worker_unittest.cpp
          order_book_feeds_manager_unittest.cpp
          ${LONGLP_PROJECT_SRC_DIR}/definitions.hpp
          ${LONGLP_PROJECT_SRC_DIR}/instrument_feeds_worker.cpp
          ${LONGLP_PROJECT_SRC_DIR}/instrument_feeds_worker.hpp
          ${LONGLP_PROJECT_SRC_DIR}/order_book_feeds_manager.hpp
          ${LONGLP_PROJECT_SRC_DIR}/order_book_feeds_manager.hpp
)

# ---- Discover tests ----
include(GoogleTest)
gtest_discover_tests(project_test)
