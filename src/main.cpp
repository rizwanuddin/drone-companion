#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <chrono>
#include <common/mavlink.h>
#include "links/UdpLink.h"
#include "links/SerialLink.h"

const double RAD_TO_DEG = 57.2957795;

// Who WE are on the MAVLink network: an onboard computer, NOT a ground station (255)
const int OUR_SYSTEM_ID = 250;
const int OUR_COMPONENT_ID = MAV_COMP_ID_ONBOARD_COMPUTER;

void sendHeartbeat(Link* link) {
    mavlink_message_t message;
    mavlink_msg_heartbeat_pack(
        OUR_SYSTEM_ID, OUR_COMPONENT_ID, &message,
        MAV_TYPE_ONBOARD_CONTROLLER,
        MAV_AUTOPILOT_INVALID,
        0, 0,
        MAV_STATE_ACTIVE);

    uint8_t sendBuffer[MAVLINK_MAX_PACKET_LEN];
    int length = mavlink_msg_to_send_buffer(sendBuffer, &message);
    link->writeBytes(sendBuffer, length);
}

void requestMessage(Link* link, int messageId, int timesPerSecond) {
    mavlink_message_t message;
    float intervalMicroseconds = 1000000.0f / timesPerSecond;

    mavlink_msg_command_long_pack(
        OUR_SYSTEM_ID, OUR_COMPONENT_ID, &message,
        1, 1,
        MAV_CMD_SET_MESSAGE_INTERVAL, 0,
        (float)messageId, intervalMicroseconds,
        0, 0, 0, 0, 0);

    uint8_t sendBuffer[MAVLINK_MAX_PACKET_LEN];
    int length = mavlink_msg_to_send_buffer(sendBuffer, &message);
    link->writeBytes(sendBuffer, length);
}

std::string fixTypeName(int fixType) {
    if (fixType == 0) return "NO GPS DETECTED";
    if (fixType == 1) return "NO FIX (searching)";
    if (fixType == 2) return "2D FIX";
    if (fixType == 3) return "3D FIX";
    return "FIX TYPE " + std::to_string(fixType);
}

void printUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  ./telemetry                         (SITL, listen on UDP 14551)" << std::endl;
    std::cout << "  ./telemetry serial <device>         (FC plugged in directly by USB)" << std::endl;
    std::cout << "  ./telemetry router [ip] [port]      (mavlink-router, default 127.0.0.1 14540)" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = "udp";
    if (argc >= 2) {
        mode = argv[1];
    }

    std::string devicePath = "";
    std::string routerIp = "127.0.0.1";
    int routerPort = 14540;

    if (mode == "serial") {
        if (argc < 3) {
            printUsage();
            return 1;
        }
        devicePath = argv[2];
    }
    if (mode == "router") {
        if (argc >= 3) {
            routerIp = argv[2];
        }
        if (argc >= 4) {
            routerPort = std::stoi(argv[3]);
        }
    }

    UdpLink listenLink(14551);
    UdpLink routerLink(routerIp, routerPort);
    SerialLink serialLink(devicePath);
    Link* link = nullptr;

    if (mode == "udp") {
        link = &listenLink;
    } else if (mode == "serial") {
        link = &serialLink;
    } else if (mode == "router") {
        link = &routerLink;
    } else {
        printUsage();
        return 1;
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

    std::chrono::steady_clock::time_point lastHeartbeatSent =
        std::chrono::steady_clock::now() - std::chrono::seconds(2);

    while (true) {
        // 1. Say hello once per second
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        double secondsSinceLastHeartbeat =
            std::chrono::duration<double>(now - lastHeartbeatSent).count();

        if (secondsSinceLastHeartbeat >= 1.0) {
            sendHeartbeat(link);
            lastHeartbeatSent = now;
        }

        // 2. Read whatever arrived (gives up after 1 second on UDP)
        int bytesReceived = link->readBytes(buffer, sizeof(buffer));
        if (bytesReceived <= 0) {
            continue;
        }

        // 3. Parse it
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
                    requestMessage(link, MAVLINK_MSG_ID_GPS_RAW_INT, 1);
                    alreadyRequested = true;
                    std::cout << "Requested ATTITUDE, POSITION and GPS_RAW from the drone" << std::endl;
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

            if (message.msgid == MAVLINK_MSG_ID_GPS_RAW_INT) {
                mavlink_gps_raw_int_t gps;
                mavlink_msg_gps_raw_int_decode(&message, &gps);

                std::cout << std::fixed << std::setprecision(7)
                          << "[GPS_RAW] " << fixTypeName(gps.fix_type)
                          << " satellites=" << (int)gps.satellites_visible
                          << " lat=" << gps.lat / 10000000.0
                          << " lon=" << gps.lon / 10000000.0 << std::endl;
            }

            if (message.msgid == MAVLINK_MSG_ID_STATUSTEXT) {
                mavlink_statustext_t statusText;
                mavlink_msg_statustext_decode(&message, &statusText);

                char text[51];
                memcpy(text, statusText.text, 50);
                text[50] = '\0';

                std::cout << "[FC MESSAGE] " << text << std::endl;
            }
        }
    }

    link->closeLink();
    return 0;
}