#include "PrometheusClient.hpp"
#include <PluginCore/Logger/Log>
#include <boost/json/serialize.hpp>
#include <boost/property_tree/ptree.hpp>
#include <exception>
#include <memory>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/json.hpp>
#include <string>

namespace json = boost::json;
using boost::property_tree::ptree;

std::string base64_encode(const std::string &data)
{
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(data.begin()), It(data.end()));
    return tmp.append((3 - data.size() % 3) % 3, '=');
}

bool PrometheusClientInfo::parse(PrometheusClientConfig &conf)
{
    url = conf.url;
    if (url.empty()) {
        R_LOG(1, "Error to parse source key url: it was empty, or parse incomplete!");
        return false;
    }
    if (conf.auth_type.value == "basic") {
        if (conf.credentials.username.value.empty() || conf.credentials.password.value.empty()) {
            R_LOG(1, "Error to parse source keys username, password: it was empty, or parse incomplete!");
            return false;
        }
        authorization =
            "Basic " + base64_encode(conf.credentials.username.value + ":" + conf.credentials.password.value);
    }
    if (conf.auth_type.value == "bearer_token") {
        if (conf.credentials.token.value.empty()) {
            R_LOG(1, "Error to parse source key token: it was empty, or parse incomplete!");
            return false;
        }
        authorization = "Bearer " + conf.credentials.token.value;
    }
    return true;
};

PrometheusClient::PrometheusClient(boost::asio::io_context &ioc, const std::vector<std::string> &metrics,
                                   const PrometheusClientInfo &info)
    : io_(ioc), metrics_(metrics)
{
    client_ = std::make_unique<d3156::AsyncHttpClient>(ioc, info.url, "", info.authorization);
}

net::awaitable<void> PrometheusClient::update()
{
    for (const auto &i : metrics_) {
        d3156::resp_dynamic_body res = co_await client_->getAsync("/api/v1/query?query=" + i, "");
        std::string body             = beast::buffers_to_string(res.body().data());
        try {
            json::value jv  = boost::json::parse(body);
            auto const &res = jv.at_pointer("/data/result").as_array();
            for (auto const &e : res) {
                auto const &m   = e.at_pointer("/metric").as_object();
                std::string key = json::serialize(m);
                auto metric     = metrics_instance.find(key);
                auto value      = std::stoll(e.at_pointer("/value/1").get_string().data());

                if (metric == metrics_instance.end()) {
                    std::vector<Metrics::Tag> tags;
                    std::string name;
                    for (auto const &[k, v] : m) {
                        if (k != "__name__")
                            tags.emplace_back(Metrics::Tag{k, v.as_string()});
                        else
                            name = v.as_string();
                    }
                    metrics_instance[key]           = std::make_unique<Metrics::Metric>(name, tags);
                    metrics_instance[key]->imported = true;
                    metrics_instance[key]->value_   = value;
                    G_LOG(5, "Create metric:" << metrics_instance[key]->toString());
                    continue;
                }
                metric->second->value_ = value;
                G_LOG(5, "Update metric:" << metric->second->toString());
            }
        } catch (std::exception &e) {
            R_LOG(0, "Error to parse metrics responce:" << e.what());
        }
    }
}
