#include "mfwpch.h"
#pragma once

#define INLINE inline __attribute((always_inline))

namespace Log {
    template <typename T>
    struct Pattern {
        explicit Pattern() {}
        static void Log(const T& value, std::string format) { 
            (void)value;
            (void)format;
        }
    };

    INLINE std::string get_format_specifier(const std::string& format, const char* default_res = "") {
        std::string result;
        if (format.empty())
            return default_res;
        if (format[0] != ':') {
            result = format.substr(0, format.find(':'));
            return result;
        }
        return default_res;
    };

    INLINE std::string get_format_flag(const std::string& format, const char* default_res = "") {
        std::string result;
        int sIndex = format.find(':');
        if (sIndex == -1)
            return default_res;
        result = format[sIndex + 1];
        if (result[0] != '<' && result[0] != '>') {
            result = '-';
            return result;
        }
        switch (result[0]) {
            case '0':
                result.clear();
                break;
            case '<':
                result = '-';
                break;
            case '>':
                result = "";
                break;
            default:
                break;
        }
        return result;
    };

    INLINE std::string get_format_width(const std::string& format, const char* default_res = "-1") {
        std::string result = default_res;
        int sIndex = format.find(':');
        if (sIndex == -1)
            return result;

        if (format[sIndex + 1] != '<' && format[sIndex + 1] != '>')
            --sIndex;

        int dIndex = format.find('.');
        if (dIndex == -1 && (int)format.size() - sIndex > 2) {
            result = format.substr(sIndex + 2);
            return result;
        }
        if (dIndex > sIndex + 2)
            result = format.substr(sIndex + 2, dIndex - sIndex - 2);
        return result;
    };

    INLINE std::string get_format_percision(const std::string& format, const char* default_res = "6") {
        std::string result = default_res;
        int dIndex = format.find('.');
        if(dIndex == -1 || dIndex == (int)format.size() - 1)
            return result;
        result = format.substr(dIndex + 1);
        return result;
    };

#define DEFINE_PATTERN_BASIC_LOG(type, dspecifier, enablePercision, dpercision, ...)\
        template <>\
        struct Pattern<type> {\
            static void Log(type value, const std::string& pattern) {\
                std::string final_format = "%";\
                final_format += get_format_flag(pattern, "-");\
                final_format += get_format_width(pattern);\
                if (enablePercision) {\
                    final_format.push_back('.');\
                    final_format += get_format_percision(pattern, dpercision);\
                }\
                final_format += get_format_specifier(pattern, dspecifier);\
                printf(final_format.c_str(), value);\
            }\
        };

#define DEFINE_PATTERN_BASIC_CHAR_LOG(type, _pattern, ...)\
        template <>\
        struct Pattern<type> {\
            static void Log(type value, const std::string& pattern) {\
                (void)pattern;\
                printf(_pattern, value);\
            }\
        };\

DEFINE_PATTERN_BASIC_LOG(int8_t, "hd", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(uint8_t, "hu", false, "1", value);

DEFINE_PATTERN_BASIC_LOG(int16_t, "hd", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(uint16_t, "u", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(int32_t, "d", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(uint32_t, "u", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(int64_t, "lld", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(uint64_t, "llu", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(long, "ld", false, "1", value);
DEFINE_PATTERN_BASIC_LOG(unsigned long, "lu", false, "1", value);

DEFINE_PATTERN_BASIC_LOG(float, "g", true, "6", value);
DEFINE_PATTERN_BASIC_LOG(double, "lg", true, "6",  value);

DEFINE_PATTERN_BASIC_CHAR_LOG(char*, "%s", value);
DEFINE_PATTERN_BASIC_CHAR_LOG(const char*, "%s", value);
DEFINE_PATTERN_BASIC_CHAR_LOG(unsigned char*, "%s", value);
DEFINE_PATTERN_BASIC_CHAR_LOG(const unsigned char*, "%s", value);

DEFINE_PATTERN_BASIC_CHAR_LOG(char, "%c", value);
DEFINE_PATTERN_BASIC_CHAR_LOG(const char, "%c", value);

    template <>
    struct Pattern<std::string> {
        static void Log(const std::string& value, const std::string& pattern) {
            (void)pattern;
            printf("%s", value.c_str());
        }
    };

}

namespace Log {
    enum class ColorPattern {
        Reset = 0,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };
    void SetColor(int colorPattern);
    void SetColor(ColorPattern colorPattern);
};

#if defined LOGGER_IMPLEMENTATION
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
    namespace Log {
        void SetColor(int colorPattern) {
            HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleTextAttribute(handle, colorPattern);
        }

        void SetColor(ColorPattern colorPattern) { 
            int pattern;
            switch (colorPattern) {
                case ColorPattern::Reset: pattern = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
                case ColorPattern::Black: pattern = 0; break;
                case ColorPattern::Red: pattern = FOREGROUND_RED; break;
                case ColorPattern::Green: pattern = FOREGROUND_GREEN; break;
                case ColorPattern::Yellow: pattern = FOREGROUND_RED | FOREGROUND_GREEN; break;
                case ColorPattern::Blue: pattern = FOREGROUND_BLUE; break;
                case ColorPattern::Magenta: pattern = FOREGROUND_RED | FOREGROUND_BLUE; break;
                case ColorPattern::Cyan: pattern = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
                case ColorPattern::White: pattern = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
                default: pattern = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
            }
            SetColor(pattern);
        }

        void ResetColor() {
            SetColor(ColorPattern::Reset);
        }
    }
#else
#include <stdlib.h>
    namespace Log {
        void SetColor(int colorPattern) { 
            std::string pattern = "\033[1;" + std::to_string(colorPattern) + "m";
            Pattern<std::string>::Log(pattern); 
        }

        void SetColor(ColorPattern colorPattern) { 
            int pattern;
            switch (colorPattern) {
                case ColorPattern::Reset: pattern = 0; break;
                case ColorPattern::Black: pattern = 30; break;
                case ColorPattern::Red: pattern = 31; break;
                case ColorPattern::Green: pattern = 32; break;
                case ColorPattern::Yellow: pattern = 33; break;
                case ColorPattern::Blue: pattern = 34; break;
                case ColorPattern::Magenta: pattern = 35; break;
                case ColorPattern::Cyan: pattern = 36; break;
                case ColorPattern::White: pattern = 37; break;
                default: pattern = 38; break;
            }
            SetColor(pattern);
        }

        void ResetColor() {
            SetColor(ColorPattern::Reset);
        }
    }
#endif
#endif

namespace Log {
    INLINE const std::string find_string_between(const std::string& str, char a, char b) {
        auto fristIndex = str.find(a);
        return str.substr(fristIndex + 1, str.find(b) - fristIndex - 1);
    }

    template <typename Arg>
    INLINE void basic_log(const std::string& pattern, const Arg& arg) {
        std::string& ptn = const_cast<std::string&>(pattern);
        auto bstart = ptn.find("{");
        Pattern<std::string>::Log(ptn.substr(0, bstart), "");
        std::string bs = find_string_between(pattern, '{', '}');
        ptn.erase(0, bstart + bs.length() + 2);

        Pattern<Arg>::Log(arg, bs);
        Pattern<std::string>::Log(ptn, "");
    }

    template <typename Arg, typename... Args>
    INLINE void basic_log(const std::string& pattern, const Arg& arg, const Args& ...args) {
        std::string& ptn = const_cast<std::string&>(pattern);
        auto bstart = ptn.find("{");
        Pattern<std::string>::Log(ptn.substr(0, bstart), "");
        std::string bs = find_string_between(pattern, '{', '}');
        ptn.erase(0, bstart + bs.length() + 2);

        Pattern<Arg>::Log(arg, bs);
        basic_log(pattern, args...);
    }

    template <typename... Args>
    INLINE void Log(ColorPattern colorPattern, const std::string& pattern, const Args& ...args) {
        SetColor(colorPattern);
        basic_log(pattern, args...);
        SetColor(ColorPattern::Reset);
    }

    template <typename... Args>
    INLINE void Log(const std::string& pattern, const Args& ...args) {
        basic_log(pattern, args...);
    }

    INLINE void basic_log(const std::string& pattern) {
        Pattern<std::string>::Log(pattern, "");
    }
    
    INLINE void Log(ColorPattern colorPattern, const std::string& pattern) {
        SetColor(colorPattern);
        basic_log(pattern);
        SetColor(ColorPattern::Reset);
    }

#define SET_LOG_COLOR_WITH_NAME(name, color)\
        template <typename... Args>\
        INLINE void name(const std::string& pattern, const Args& ...args) {\
            Log(color, pattern, args...);\
        }\

SET_LOG_COLOR_WITH_NAME(Trace, ColorPattern::White);
SET_LOG_COLOR_WITH_NAME(Debug, ColorPattern::Green);
SET_LOG_COLOR_WITH_NAME(Info, ColorPattern::Cyan);
SET_LOG_COLOR_WITH_NAME(Warn, ColorPattern::Yellow);
SET_LOG_COLOR_WITH_NAME(Error, ColorPattern::Magenta);
SET_LOG_COLOR_WITH_NAME(Fatal, ColorPattern::Red);
}

