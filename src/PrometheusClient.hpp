#pragma once

#include "EasyHttpClient.hpp"
#include "Metrics.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct PrometheusClientInfo {

    bool parse(const boost::property_tree::ptree &pt);
    std::string url;
    std::string authorization;
};

class PrometheusClient
{
    boost::asio::io_context &io_;
    std::unique_ptr<d3156::EasyHttpClient> client_;
    std::vector<std::string> metrics_;
    std::unordered_map<std::string, std::unique_ptr<Metrics::Metric>> metrics_instance;

public:
    PrometheusClient(boost::asio::io_context &ioc, const std::vector<std::string> &metrics,
                     const PrometheusClientInfo &info);

    void update();
};