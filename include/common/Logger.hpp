#pragma once

#include <iostream>
#include <string>

// ANSI color codes for terminal output
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_GREEN "\033[32m"
#define COLOR_CYAN "\033[36m"
#define COLOR_GRAY "\033[90m"

// Extract filename from full path
inline const char *extract_filename(const char *path) {
  const char *file = path;
  while (*path) {
    if (*path++ == '/') {
      file = path;
    }
  }
  return file;
}

// Log levels
enum class LogLevel { L_DEBUG, L_INFO, L_WARN, L_ERROR };

// Core logging function
inline void log_message(LogLevel level, const char *file, int line,
                        const std::string &message) {
  const char *level_str;
  const char *color;

  switch (level) {
  case LogLevel::L_DEBUG:
    level_str = "DEBUG";
    color = COLOR_GRAY;
    break;
  case LogLevel::L_INFO:
    level_str = "INFO ";
    color = COLOR_GREEN;
    break;
  case LogLevel::L_WARN:
    level_str = "WARN ";
    color = COLOR_YELLOW;
    break;
  case LogLevel::L_ERROR:
    level_str = "ERROR";
    color = COLOR_RED;
    break;
  }

  std::cerr << color << "[" << level_str << "]" << COLOR_RESET << " ["
            << COLOR_CYAN << extract_filename(file) << ":" << line
            << COLOR_RESET << "] " << message << std::endl;
}

// Logging macros with file and line information
// Logging macros with file and line information
#ifdef DEBUG
#define LOG_DEBUG(msg) log_message(LogLevel::L_DEBUG, __FILE__, __LINE__, msg)
#else
#define LOG_DEBUG(msg) ((void)0)
#endif

#define LOG_INFO(msg) log_message(LogLevel::L_INFO, __FILE__, __LINE__, msg)
#define LOG_WARN(msg) log_message(LogLevel::L_WARN, __FILE__, __LINE__, msg)
#define LOG_ERROR(msg) log_message(LogLevel::L_ERROR, __FILE__, __LINE__, msg)
