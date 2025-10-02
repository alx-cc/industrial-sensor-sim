#include "industrial/MqttPublisher.hpp"

// Using the Eclipse Paho MQTT C library (not the C++ wrapper) since this project targets no-exceptions builds
// and the C++ wrapper reports errors via exceptions.

#if defined(PAHO_MQTT_C_AVAILABLE)
#include <cstring>
#include <MQTTClient.h>
#endif

namespace industrial {

MqttPublisher::MqttPublisher() = default;
MqttPublisher::~MqttPublisher() { disconnect(); }

bool MqttPublisher::connect(const std::string& brokerUri, const std::string& clientId, int keepAliveSec) {
#if defined(PAHO_MQTT_C_AVAILABLE)
    if (connected_) return true;
    MQTTClient c = nullptr;
    int rc = MQTTClient_create(&c, brokerUri.c_str(), clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, nullptr);
    if (rc != MQTTCLIENT_SUCCESS) return false;

    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    opts.keepAliveInterval = keepAliveSec > 0 ? keepAliveSec : 60;
    opts.cleansession = 1;
    rc = MQTTClient_connect(c, &opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        MQTTClient_destroy(&c);
        return false;
    }
    client_ = c;
    connected_ = true;
    return true;
#else
    (void)brokerUri; (void)clientId; (void)keepAliveSec;
    return false;
#endif
}

bool MqttPublisher::publish(const std::string& topic, const void* payload, size_t len, int qos, bool retain) {
#if defined(PAHO_MQTT_C_AVAILABLE)
    if (!connected_ || client_ == nullptr) return false;
    MQTTClient_message msg = MQTTClient_message_initializer;
    msg.payload = const_cast<void*>(payload);
    msg.payloadlen = (int)len;
    msg.qos = qos;
    msg.retained = retain ? 1 : 0;
    MQTTClient_deliveryToken token = 0;
    int rc = MQTTClient_publishMessage((MQTTClient)client_, topic.c_str(), &msg, &token);
    if (rc != MQTTCLIENT_SUCCESS) return false;
    if (qos > 0) {
        rc = MQTTClient_waitForCompletion((MQTTClient)client_, token, 5000L);
        if (rc != MQTTCLIENT_SUCCESS) return false;
    }
    return true;
#else
    (void)topic; (void)payload; (void)len; (void)qos; (void)retain;
    return false;
#endif
}

void MqttPublisher::disconnect() {
#if defined(PAHO_MQTT_C_AVAILABLE)
    if (client_ != nullptr) {
        if (connected_) {
            MQTTClient_disconnect((MQTTClient)client_, 2000);
        }
        MQTTClient_destroy((MQTTClient*)&client_);
        client_ = nullptr;
    }
    connected_ = false;
#endif
}

bool MqttPublisher::is_connected() const { return connected_; }

} // namespace industrial
