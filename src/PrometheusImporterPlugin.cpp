#include "PrometheusImporterPlugin.hpp"
#include "PrometheusClient.hpp"
#include <PluginCore/Logger/Log>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <MetricsModel/MetricsModel>

void PrometheusImporterPlugin::registerArgs(d3156::Args::Builder &bldr)
{
    bldr.setVersion(FULL_NAME).addOption(configPath, "PrometheusPath",
                                         "path to config for PrometheusImporterPlugin.json");
}

void PrometheusImporterPlugin::registerModels(d3156::PluginCore::ModelsStorage &models)
{
    MetricsModel::instance() = models.registerModel<MetricsModel>();
}

void PrometheusImporterPlugin::postInit()
{
    parseSettings();
    update_timer = std::make_unique<boost::asio::steady_timer>(MetricsModel::instance()->getIO(),
                                                               std::chrono::seconds(import_timer));
    update_timer->async_wait([this](const boost::system::error_code &ec) { this->onTimer(ec); });
}

// ABI required by d3156::PluginCore::Core (dlsym uses exact names)
extern "C" d3156::PluginCore::IPlugin *create_plugin() { return new PrometheusImporterPlugin(); }

extern "C" void destroy_plugin(d3156::PluginCore::IPlugin *p) { delete p; }

using boost::property_tree::ptree;
namespace fs = std::filesystem;

void PrometheusImporterPlugin::parseSettings()
{
    if (!fs::exists(configPath)) {
        Y_LOG(0, "Config file " << configPath << " not found. Creating default config...");
        fs::create_directories(fs::path(configPath).parent_path());
        ptree root;
        root.put("reconnect_timeout", 45000);
        root.put("import_timer", 15);

        ptree sourcesArray;
        ptree source1;
        source1.put("url", "http://prometheus.prod.internal:9090");
        source1.put("auth_type", "bearer_token");
        source1.put("token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
        sourcesArray.push_back(std::make_pair("", source1));
        ptree source2;
        source2.put("url", "https://prometheus.staging.example.com");
        source2.put("auth_type", "basic");
        source2.put("username", "monitor");
        source2.put("password", "secure_password_123");
        sourcesArray.push_back(std::make_pair("", source2));
        ptree source3;
        source3.put("url", "http://localhost:9090");
        source3.put("auth_type", "none");
        sourcesArray.push_back(std::make_pair("", source3));
        root.add_child("sources", sourcesArray);

        ptree metrics_array;
        metrics_array.push_back(std::make_pair("", ptree("up")));
        metrics_array.push_back(std::make_pair("", ptree("push_time_seconds")));
        root.add_child("metrics", metrics_array);

        try {
            write_json(configPath, root);
            G_LOG(0, "Default config created at " << configPath);
        } catch (const std::exception &e) {
            R_LOG(0, "Error writing config file: " << e.what());
            throw;
        }
        return;
    }
    try {
        ptree pt;
        read_json(configPath, pt);
        reconnect_timeout = pt.get<size_t>("reconnect_timeout");
        import_timer      = pt.get<size_t>("import_timer");
        for (const auto &item : pt.get_child("metrics")) metrics.push_back(item.second.get_value<std::string>());
        for (auto &n : pt.get_child("sources", ptree{})) {
            PrometheusClientInfo info;
            if (info.parse(n.second))
                instances.push_back(
                    std::make_unique<PrometheusClient>(MetricsModel::instance()->getIO(), metrics, info));
        }
        if (instances.empty()) R_LOG(1, "in " << configPath << " key sources is empty, or parse incomplete!");
    } catch (std::exception e) {
        R_LOG(1, "error on load config " << configPath << " " << e.what());
    }
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
        update_timer->expires_after(std::chrono::seconds(import_timer));
        update_timer->async_wait([this](const boost::system::error_code &e) { onTimer(e); });
    }
}
