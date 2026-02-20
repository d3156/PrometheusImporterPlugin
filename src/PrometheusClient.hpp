#pragma once

#include <EasyHttpLib/AsyncHttpClient>
#include <MetricsModel/Metrics>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <BaseConfig>

struct PrometheusClientConfig : public d3156::Config {
    PrometheusClientConfig(d3156::Config *parent) : d3156::Config("", parent) {}
    CONFIG_STRING(url, "");
    CONFIG_ENUM(auth_type, "none", "none|basic|bearer_token");
    struct Credentials : public d3156::Config {
        Credentials(d3156::Config *parent) : d3156::Config("credentials", parent) {}
        CONFIG_STRING(username, "monitor");
        CONFIG_STRING(password, "secure_password_123");
        CONFIG_STRING(token, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
    } credentials = Credentials(this);
};

struct PrometheusClientInfo {

    bool parse(PrometheusClientConfig &conf);
    std::string url;
    std::string authorization;
};

class PrometheusClient
{
    boost::asio::io_context &io_;
    std::unique_ptr<d3156::AsyncHttpClient> client_;
    std::vector<std::string> metrics_;
    std::unordered_map<std::string, std::unique_ptr<Metrics::Metric>> metrics_instance;

public:
    PrometheusClient(boost::asio::io_context &ioc, const std::vector<std::string> &metrics,
                     const PrometheusClientInfo &info);

    net::awaitable<void> update();
};