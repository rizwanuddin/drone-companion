# Pi mavlink-router setup

Raspberry Pi Zero 2 W as companion computer for an ArduPilot quad (MicoAir405v2, Copter 4.7.2).
Xbox -> QGroundControl (Mac) --UDP/Wi-Fi--> Pi: mavlink-router --USB--> FC.
Local apps connect to udp://127.0.0.1:14540.

## Layout
- `pi/etc/mavlink-router/main.conf`: router config (copy to `/etc/mavlink-router/`). Set the qgc `Address` to your laptop IP.
- `pi/etc/systemd/system/`: service plus restart override.
- `tools/`: pymavlink-free MAVLink2 test scripts (sensor health, param read). Run on the Pi with the router up.

## Pi setup notes
- Build mavlink-router from source (github.com/mavlink-router/mavlink-router), install to `/usr/bin/mavlink-routerd`.
- `/boot/firmware/config.txt`, under `[all]`: `dtoverlay=dwc2,dr_mode=host`. Without it the legacy dwc_otg driver hung the Pi when the FC enumerated.
- `sudo systemctl enable --now mavlink-router`.

## FC notes
- Copter 4.7.2 needed: QMC5883P compass (I2C 0x2C) is excluded on 1MB boards in 4.5.7/4.6.0.
- GPS connector: GND, 5V, TX3, RX3, SCL, SDA. UART TX/RX crossed, I2C SDA/SCL straight.

## Safety
Props off for every bench test. Nothing here arms the vehicle.
