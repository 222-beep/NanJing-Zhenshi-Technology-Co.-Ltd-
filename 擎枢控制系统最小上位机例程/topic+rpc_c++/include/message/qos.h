#pragma once

// Wrapper only: do not add include/message to global include_directories, or this
// file name would shadow the Windows SDK <qos.h> required by winsock2.h.
#include "message/message_bus_qos.h"
