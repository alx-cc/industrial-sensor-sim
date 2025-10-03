/**
 * @file MqttPublisher.cpp
 * @brief Exception-free MQTT publishing backend implemented with Eclipse Paho MQTT C.
 *
 * @details
 * Implements the runtime of industrial::MqttPublisher using the Paho C API:
 * - Connects with clean session and configurable keep-alive.
 * - Publishes with caller-specified QoS and retain flags.
 * - For QoS > 0, blocks until delivery completion (up to ~5s).
 * - Ensures orderly disconnect on request and in the destructor (~2s timeout).
 *
 * Build-time behavior:
 * - If PAHO_MQTT_C_AVAILABLE is defined, uses the Paho C API directly.
 * - Otherwise, methods act as stubs (feature unavailable) and return false.
 *
 * Error handling:
 * - No exceptions; all operations return boolean success. Callers must check results.
 *
 * Notes:
 * - Manages MQTTClient lifecycle internally.
 * - Not inherently thread-safe; external synchronization may be required.
 * - uses Paho C lib (not the C++ wrapper) since this project targets no-exceptions builds
 *   and the C++ wrapper reports errors via exceptions.
 */
#include "industrial/MqttPublisher.hpp"

#if defined(PAHO_MQTT_C_AVAILABLE)
#include <cstring>
#include <MQTTClient.h>
#endif

namespace industrial {

MqttPublisher::MqttPublisher() = default;
MqttPublisher::~MqttPublisher() { disconnect(); }

/**
 * @brief Connects to an MQTT broker and initializes the internal MQTT client.
 *
 * Creates a new Eclipse Paho C MQTT client and attempts a clean-session connection
 * to the specified broker. If already connected, the call is a no-op and returns true.
 * The keep-alive interval is set to keepAliveSec if > 0, otherwise defaults to 60 seconds.
 * On success, updates internal state and retains the client handle; on failure, cleans up.
 *
 * @note Requires compilation with PAHO_MQTT_C_AVAILABLE. When not available, the function
 *       performs no connection and returns false.
 */
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

/**
 * @brief Publish a message to the specified MQTT topic.
 *
 * Publishes the given payload using the configured MQTT client with the provided
 * QoS and retain settings. For QoS 1 or 2, the call blocks until delivery completes
 * or a 5-second timeout elapses.
 *
 * return true on success; false if the client is not connected, if publishing or completion
 * fails, or when built without Paho MQTT C support.
 *
 * @note When PAHO_MQTT_C_AVAILABLE is not defined, this function is a stub that returns false.
 */
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

// Disconnects from the MQTT broker if connected, destroys the client handle, and resets the connection state.
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
