#pragma once
#include "PrometheusClient.hpp"
#include <MetricsModel/Metrics>
#include <PluginCore/IPlugin.hpp>
#include <PluginCore/IModel.hpp>
#include <memory>
#include <vector>

class PrometheusImporterPlugin final : public d3156::PluginCore::IPlugin
{
    std::string configPath = "./configs/PrometheusImporter.json";
    std::vector<std::string> metrics;

    size_t reconnect_timeout = 8000;
    size_t import_timer      = 15;
    std::vector<std::unique_ptr<PrometheusClient>> instances;
    std::unique_ptr<boost::asio::steady_timer> update_timer;

public:
    void registerArgs(d3156::Args::Builder &bldr) override;

    void registerModels(d3156::PluginCore::ModelsStorage &models) override;

    void postInit() override;

private:
    void onTimer(const boost::system::error_code &ec);

    void parseSettings();

    virtual ~PrometheusImporterPlugin();
};
