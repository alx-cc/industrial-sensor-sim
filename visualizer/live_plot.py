#!/usr/bin/env python3
"""Live plot for sensor_sim MQTT output.

Expects CSV payloads: tempC,avgTempC,pressKPa,avgPressKPa
Environment variables (optional):
  MQTT_BROKER (default 127.0.0.1)
  MQTT_PORT   (default 1883)
  MQTT_TOPIC  (default sensors/demo/readings)

Install deps:
  pip install paho-mqtt matplotlib

Run:
  python3 visualizer/live_plot.py
"""
import os
import sys
import time
from collections import deque

try:
    import paho.mqtt.client as mqtt
    import matplotlib.pyplot as plt
except ImportError as e:
    print("Missing dependency: {}. Run 'pip install paho-mqtt matplotlib'".format(e.name), file=sys.stderr)
    sys.exit(1)

BROKER = os.getenv("MQTT_BROKER", "127.0.0.1")
PORT = int(os.getenv("MQTT_PORT", "1883"))
TOPIC = os.getenv("MQTT_TOPIC", "sensors/demo/readings")
MAX_POINTS = 300

temps = deque(maxlen=MAX_POINTS)
temps_avg = deque(maxlen=MAX_POINTS)
press = deque(maxlen=MAX_POINTS)
press_avg = deque(maxlen=MAX_POINTS)
last_update = 0.0


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe(TOPIC)
    else:
        print(f"Connect failed: rc={rc}", file=sys.stderr)


def on_message(client, userdata, msg):
    global last_update
    try:
        parts = msg.payload.decode().strip().split(",")
        if len(parts) != 4:
            return
        t, ta, p, pa = map(float, parts)
        temps.append(t)
        temps_avg.append(ta)
        press.append(p)
        press_avg.append(pa)
        last_update = time.time()
    except Exception:
        pass


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.loop_start()

plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True)
fig.suptitle(f"Live MQTT: {TOPIC}@{BROKER}:{PORT}")
ax1.set_ylabel("Temp (C)")
ax2.set_ylabel("Pressure (kPa)")
ax2.set_xlabel("Samples")

(t_line,) = ax1.plot([], [], label="T", linewidth=0.9)
(ta_line,) = ax1.plot([], [], label="T avg", linewidth=0.9)
(p_line,) = ax2.plot([], [], label="P", linewidth=0.9)
(pa_line,) = ax2.plot([], [], label="P avg", linewidth=0.9)
ax1.legend(loc="upper right", fontsize="x-small")
ax2.legend(loc="upper right", fontsize="x-small")

status_txt = ax2.text(0.01, 0.01, "", transform=ax2.transAxes, fontsize=8, va="bottom")

REFRESH_SEC = 0.25
IDLE_WARN_SEC = 5.0

try:
    while True:
        plt.pause(REFRESH_SEC)
        if temps:
            xs = range(len(temps))
            t_line.set_data(xs, temps)
            ta_line.set_data(xs, temps_avg)
        if press:
            xs = range(len(press))
            p_line.set_data(xs, press)
            pa_line.set_data(xs, press_avg)
        for ax in (ax1, ax2):
            ax.relim()
            ax.autoscale_view()
        idle = time.time() - last_update
        if idle > IDLE_WARN_SEC:
            status_txt.set_text(f"Idle {idle:.1f}s (no new msgs)")
        else:
            status_txt.set_text("")
except KeyboardInterrupt:
    pass
finally:
    client.loop_stop()
    client.disconnect()
