add_executable(order-book-watcher)

set(LONGLP_CONFIG_DATA_DIR "${LONGLP_PROJECT_DATA_DIR}")
configure_file(
  config.hpp.tmpl ${LONGLP_PROJECT_GEN_DIR}/longlp_config.hpp @ONLY
)

target_compile_options(
  order-book-watcher PRIVATE ${LONGLP_DESIRED_COMPILE_OPTIONS}
)
target_compile_features(
  order-book-watcher PRIVATE ${LONGLP_DESIRED_COMPILE_FEATURES}
)
target_include_directories(
  order-book-watcher PRIVATE ${LONGLP_PROJECT_SRC_DIR}
                             ${LONGLP_PROJECT_GEN_DIR}
)
target_link_libraries(
  order-book-watcher
  PRIVATE nlohmann_json::nlohmann_json fmt::fmt Taskflow::Taskflow
          Threads::Threads
          # target for each solution
)
target_sources(
  order-book-watcher
  PRIVATE main.cpp definitions.hpp instrument_feeds_worker.cpp
          instrument_feeds_worker.hpp order_book_feeds_manager.cpp
          order_book_feeds_manager.hpp
)

add_dependencies(order-book-watcher copy_data)
