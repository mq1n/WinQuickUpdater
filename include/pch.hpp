#define _CRT_SECURE_NO_WARNINGS
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <Windowsx.h>
#include <dinput.h>
#include <dwmapi.h>
#include <wuapi.h>
#include <wuerror.h>
#include <comdef.h>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <csignal>
#include <thread>
#include <atomic>
#include <ctime>

#include <cxxopts.hpp>
#include <fmt/format.h>
using namespace std::string_literals;

#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
using namespace rapidjson;
