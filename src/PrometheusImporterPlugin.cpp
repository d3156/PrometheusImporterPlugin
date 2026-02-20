#include "PrometheusImporterPlugin.hpp"
#include "PrometheusClient.hpp"
#include <PluginCore/Logger/Log>
#include <ConfiguratorModel>
#include <memory>
#include <string>
#include <MetricsModel/MetricsModel>

void PrometheusImporterPlugin::registerArgs(d3156::Args::Builder &bldr) { bldr.setVersion(FULL_NAME); }

void PrometheusImporterPlugin::registerModels(d3156::PluginCore::ModelsStorage &models)
{
    MetricsModel::instance() = models.registerModel<MetricsModel>();
    models.registerModel<ConfiguratorModel>()->registerConfig("PrometheusImporterPlugin", conf);
}

void PrometheusImporterPlugin::postInit()
{
    parseSettings();
    update_timer = std::make_unique<boost::asio::steady_timer>(MetricsModel::instance()->getIO(),
                                                               std::chrono::seconds(conf.import_timer.value));
    update_timer->async_wait([this](const boost::system::error_code &ec) { this->onTimer(ec); });
}

void PrometheusImporterPlugin::parseSettings()
{
    for (auto &i : conf.sources.items) {
        PrometheusClientInfo info;
        if (info.parse(i))
            instances.push_back(
                std::make_unique<PrometheusClient>(MetricsModel::instance()->getIO(), conf.metrics.items, info));
    }
    if (instances.empty()) R_LOG(1, " sources is empty, or parse incomplete!");
}

PrometheusImporterPlugin::~PrometheusImporterPlugin()
{
    if (update_timer) update_timer->cancel();
}

void PrometheusImporterPlugin::onTimer(const boost::system::error_code &ec)
{
    if (!ec) {
        for (auto &instance : instances)
            net::co_spawn(MetricsModel::instance()->getIO(), instance->update(), net::detached);
        update_timer->expires_after(std::chrono::seconds(conf.import_timer.value));
        update_timer->async_wait([this](const boost::system::error_code &e) { onTimer(e); });
    }
}

// ABI required by d3156::PluginCore::Core (dlsym uses exact names)
extern "C" d3156::PluginCore::IPlugin *create_plugin() { return new PrometheusImporterPlugin(); }

extern "C" void destroy_plugin(d3156::PluginCore::IPlugin *p) { delete p; }