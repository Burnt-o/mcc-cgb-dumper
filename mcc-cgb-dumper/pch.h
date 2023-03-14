#ifndef PCH_H
#define PCH_H

// Windows 
#define WIN32_LEAN_AND_MEAN   
#define NOMINMAX // windows.h min and max macros conflict with date.h
#include <Windows.h>  


#include <iostream>
#include <vector>
#include <cstdint>
#include <optional>
#include <string>
#include <mutex>

#include <libloaderapi.h>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <iomanip>


#include <functional>


#include <nlohmann/json.hpp>

#include <date/date.h>

#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Initializers/ConsoleInitializer.h>

#endif //PCH_H