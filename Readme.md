# PrometheusImporterPlugin

Плагин для проекта PluginCore. Выполняет импорт метрик из Prometheus в MetricsModel

Файл конфигурации должен лежать в папке с загрузчиком модулей `./configs/PrometheusImporterPlugin.json`
Все метрики будут помечены флагом imorted. Т.к. exporter может не экспортировать импортированные метрики, если это потребуется.
```json 
{
  "reconnect_timeout": 45000,
  "sources": [
    {
      "url": "http://prometheus.prod.internal:9090",
      "auth_type": "bearer_token",
      "credentials": {
        "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
      }
    },
    {
      "url": "https://prometheus.staging.example.com",
      "auth_type": "basic",
      "credentials": {
        "username": "monitor",
        "password": "secure_password_123"
      }
    },
    {
      "url": "http://localhost:9090",
      "auth_type": "none"
    }
  ]
}
```
