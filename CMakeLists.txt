cmake_minimum_required(VERSION 3.16)
project(PrometheusImporterPlugin VERSION 1.1.1 LANGUAGES CXX)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../tools/workspace.cmake")
create_target(plugin)

dependency("../EasyHttpLib" EasyHttpLib)
dependency("../MetricsModel" MetricsModel)

find_package(Boost 1.8 REQUIRED COMPONENTS json thread system)
target_link_libraries(PrometheusImporterPlugin PRIVATE Boost::system Boost::thread Boost::json)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
target_link_libraries(PrometheusImporterPlugin PUBLIC OpenSSL::SSL OpenSSL::Crypto)