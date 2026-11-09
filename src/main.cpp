#include <iostream>
#include <iomanip>
#include <string>
#include <common/mavlink.h>
#include "links/UdpLink.h"
#include "links/SerialLink.h"

const double RAD_TO_DEG = 57.2957795;

void requestMessage(Link* link, int messageId, int timesPerSecond) {
    mavlink_message_t message;
    float intervalMicroseconds = 1000000.0f / timesPerSecond;

    mavlink_msg_command_long_pack(
        255, 190, &message,
        1, 1,
        MAV_CMD_SET_MESSAGE_INTERVAL, 0,
        (float)messageId, intervalMicroseconds,
        0, 0, 0, 0, 0);

    uint8_t sendBuffer[MAVLINK_MAX_PACKET_LEN];
    int length = mavlink_msg_to_send_buffer(sendBuffer, &message);
    link->writeBytes(sendBuffer, length);
}

int main(int argc, char* argv[]) {
    std::string mode = "udp";
    std::string devicePath = "";
    if (argc >= 2) {
        mode = argv[1];
    }
    if (argc >= 3) {
        devicePath = argv[2];
    }

    UdpLink udpLink(14551);
    SerialLink serialLink(devicePath);
    Link* link = &udpLink;

    if (mode == "serial") {
        if (devicePath == "") {
            std::cout << "Usage: ./build/telemetry serial /dev/cu.usbmodemXXXX" << std::endl;
            return 1;
        }
        link = &serialLink;
    }

    if (link->openLink() == false) {
        std::cout << "Could not open link" << std::endl;
        return 1;
    }
    std::cout << "Connected using " << mode << ". Waiting for drone..." << std::endl;

    uint8_t buffer[2048];
    mavlink_message_t message;
    mavlink_status_t status;
    bool alreadyRequested = false;

    while (true) {
        int bytesReceived = link->readBytes(buffer, sizeof(buffer));
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

                if (alreadyRequested == false) {
                    requestMessage(link, MAVLINK_MSG_ID_ATTITUDE, 4);
                    requestMessage(link, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 2);
                    alreadyRequested = true;
                    std::cout << "Requested ATTITUDE and POSITION from the drone" << std::endl;
                }
            }

            if (message.msgid == MAVLINK_MSG_ID_ATTITUDE) {
                mavlink_attitude_t attitude;
                mavlink_msg_attitude_decode(&message, &attitude);

                std::cout << std::fixed << std::setprecision(1)
                          << "[ATTITUDE] roll=" << attitude.roll * RAD_TO_DEG
                          << " pitch=" << attitude.pitch * RAD_TO_DEG
                          << " yaw=" << attitude.yaw * RAD_TO_DEG << std::endl;
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

    link->closeLink();
    return 0;
}