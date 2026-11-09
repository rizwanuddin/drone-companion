#include <iostream>
#include <common/mavlink.h>
#include "links/UdpLink.h"
#include <iomanip>

int main() {
    UdpLink link(14551);

    if (link.openLink() == false) {
        std::cout << "Could not open link" << std::endl;
        return 1;
    }
    std::cout << "Listening for drone on UDP port 14551..." << std::endl;

    uint8_t buffer[2048];
    mavlink_message_t message;
    mavlink_status_t status;

    while (true) {
        int bytesReceived = link.readBytes(buffer, sizeof(buffer));
        if (bytesReceived <= 0) {
            continue;
        }

        for (int i = 0; i < bytesReceived; i++) {
            bool gotFullMessage = mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &message, &status);
            if (gotFullMessage == false) {
                continue;
            }

            if (message.sysid != 1) {
                continue;
            }

            if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                mavlink_heartbeat_t heartbeat;
                mavlink_msg_heartbeat_decode(&message, &heartbeat);

                bool isArmed = false;
                if (heartbeat.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) {
                    isArmed = true;
                }

                std::cout << "[HEARTBEAT] armed=" << (isArmed ? "yes" : "no")
                          << " mode=" << heartbeat.custom_mode << std::endl;
            }

            if (message.msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
                mavlink_global_position_int_t position;
                mavlink_msg_global_position_int_decode(&message, &position);

                double latitude = position.lat / 10000000.0;
                double longitude = position.lon / 10000000.0;
                double altitude = position.relative_alt / 1000.0;

                std::cout << std::fixed << std::setprecision(7)
                          << "[POSITION] lat=" << latitude
                          << " lon=" << longitude
                          << std::setprecision(2)
                          << " alt=" << altitude << "m" << std::endl;
            }
        }
    }

    link.closeLink();
    return 0;
}