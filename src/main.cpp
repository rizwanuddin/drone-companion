#include <iostream>
#include <common/mavlink.h>

int main() {
    std::cout << "MAVLink library found!" << std::endl;

    std::cout << "HEARTBEAT message id: " << MAVLINK_MSG_ID_HEARTBEAT << std::endl;
    std::cout << "ATTITUDE message id: " << MAVLINK_MSG_ID_ATTITUDE << std::endl;
    std::cout << "GLOBAL_POSITION_INT message id: " << MAVLINK_MSG_ID_GLOBAL_POSITION_INT << std::endl;

    return 0;
}