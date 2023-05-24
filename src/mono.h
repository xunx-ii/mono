#pragma once

#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_no_tls_client.hpp"

#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "websocketpp/common/memory.hpp"
#include "websocketpp/common/thread.hpp"

#include "api.h"
#include "queue.h"

#include <chrono>
#include <codecvt>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <vector>