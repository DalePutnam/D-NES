#pragma once

#include <map>

constexpr int ERROR_LOAD_GAME_AFTER_START = 1;
constexpr int ERROR_LOAD_GAME_FAILED = 2;
constexpr int ERROR_SET_WINDOW_HANDLE_AFTER_START = 3;
constexpr int ERROR_SET_CALLBACK_AFTER_START = 4;
constexpr int ERROR_FAILED_TO_OPEN_CPU_LOG_FILE = 5;
constexpr int ERROR_UNIMPLEMENTED = 6;
constexpr int ERROR_START_WITHOUT_GAME_LOADED = 7;
constexpr int ERROR_START_ALREADY_STARTED = 8;
constexpr int ERROR_START_AFTER_STOP = 9;
constexpr int ERROR_START_AFTER_ERROR = 13;
constexpr int ERROR_STOP_ALREADY_STOPPED = 11;
constexpr int ERROR_STOP_NOT_STARTED = 12;
constexpr int ERROR_STOP_AFTER_ERROR = 13;
constexpr int ERROR_RUNTIME_ERROR = 14;
constexpr int ERROR_STATE_SAVE_NOT_RUNNING = 15;
constexpr int ERROR_STATE_LOAD_NOT_RUNNING = 16;
constexpr int ERROR_STATE_SAVE_LOAD_FILE_ERROR = 17;

static const std::map<int, std::string> ERROR_CODE_TO_MESSAGE_MAP
{
    {ERROR_LOAD_GAME_AFTER_START, "Cannot load a game after the emulator has started"},
    {ERROR_LOAD_GAME_FAILED, "Failed to load ROM file"},
    {ERROR_SET_WINDOW_HANDLE_AFTER_START, "Cannot set the window handle after the emulator has started"},
    {ERROR_SET_CALLBACK_AFTER_START, "Cannot set the callback object after the emulator has started"},
    {ERROR_FAILED_TO_OPEN_CPU_LOG_FILE, "Could not open CPU log file"},
    {ERROR_UNIMPLEMENTED, "The functionality is not yet implementsd"},
    {ERROR_START_WITHOUT_GAME_LOADED, "The emulator cannot be started until a game is loaded"},
    {ERROR_START_ALREADY_STARTED, "The emulator has already been started"},
    {ERROR_START_AFTER_STOP, "The emulator cannot be restarted once it has been stopped"},
    {ERROR_START_AFTER_ERROR, "The emulator is in an error state and cannot be started"},
    {ERROR_STOP_ALREADY_STOPPED, "The emulator has already been stopped"},
    {ERROR_STOP_NOT_STARTED, "The emulator has not been started"},
    {ERROR_STOP_AFTER_ERROR, "The emulator is in an error state and is already stopped"},
    {ERROR_RUNTIME_ERROR, "The emulator encountered a fatal error while running"},
    {ERROR_STATE_SAVE_NOT_RUNNING, "Cannot save a state while the emulator is not running"},
    {ERROR_STATE_LOAD_NOT_RUNNING, "Cannot load a state while the emulator is not running"},
    {ERROR_STATE_SAVE_LOAD_FILE_ERROR, "Failed to open save state file"}
};