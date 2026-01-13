#pragma once

#include <cstdint>

namespace Lawnch::IPC::Constants {

constexpr uint8_t CMD_PING = 0x01;
constexpr uint8_t CMD_KILL = 0x02;
constexpr uint8_t RESP_OK = 0x00;
constexpr uint8_t RESP_ERROR = 0xFF;

} // namespace Lawnch::IPC::Constants
