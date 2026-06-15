#include "easy_json.h"
#include <cstdint>

struct RespDemo {
    int32_t return_code;
    int32_t subcmd_index;
    std::string return_message;
    //double vel;
    //double toq;
    //double position;
    JSON_HELP(subcmd_index, return_code, return_message);
    //JSON_HELP(subcmd_index);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RespDemo, return_code, subcmd_index, return_message)