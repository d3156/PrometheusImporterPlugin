#pragma once
#include "PrometheusClient.hpp"
#include <MetricsModel/Metrics>
#include <PluginCore/IPlugin>
#include <PluginCore/IModel>
#include <memory>
#include <string>
#include <vector>

class PrometheusImporterPlugin final : public d3156::PluginCore::IPlugin
{
    std::vector<std::unique_ptr<PrometheusClient>> instances;
    std::unique_ptr<boost::asio::steady_timer> update_timer;

public:
    struct PrometheusImporterConfig : d3156::Config {
        PrometheusImporterConfig() : d3156::Config("") {}
        CONFIG_ARRAY(metrics, std::string);
        CONFIG_UINT(reconnect_timeout, 8000);
        CONFIG_UINT(import_timer, 15);
        CONFIG_ARRAY(sources, PrometheusClientConfig);
    } conf;

    void registerArgs(d3156::Args::Builder &bldr) override;

    void registerModels(d3156::PluginCore::ModelsStorage &models) override;

    void postInit() override;

private:
    void onTimer(const boost::system::error_code &ec);

    void parseSettings();

    virtual ~PrometheusImporterPlugin();
};
