#pragma once

#include <string>

namespace industrial {

class MqttPublisher {
public:
    MqttPublisher();
    ~MqttPublisher();

    // Connect to broker, e.g., brokerUri="tcp://localhost:1883"
    // keepAliveSec typical 60.
    bool connect(const std::string& brokerUri, const std::string& clientId, int keepAliveSec);

    // Publish payload to topic. QoS: 0 or 1. retain: false by default.
    bool publish(const std::string& topic, const void* payload, size_t len, int qos, bool retain);

    void disconnect();
    bool is_connected() const;

private:
    void* client_ = 0; // opaque MQTTClient*
    bool connected_ = false;
};

} // namespace industrial
