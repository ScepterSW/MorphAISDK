#pragma once
#include <string>

#define __PURE_VIRTUAL_FUNC__           = 0

enum AlgLedColor
{
    ALG_LED_OFF   = -1,
    ALG_LED_RED   = 0,
    ALG_LED_GREEN = 1,
    ALG_LED_BLUE  = 2,
    ALG_LED_VIOLET= 3,
};

enum AlgGpioLevel
{
    ALG_GPIO_LOW  = 0,
    ALG_GPIO_HIGH = 1,
};

enum AlgHelperRet
{
    ALG_HELPER_RET_OK  =  0,
    ALG_HELPER_RET_ERR = -1
};

class AlgLedOut
{
public:
    virtual AlgHelperRet get_supported_count(int32_t *p_count) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet set_enabled(int32_t index, uint8_t enabled)    __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet get_enabled(int32_t index, uint8_t *p_enabled) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet set_color(int32_t index, AlgLedColor color) __PURE_VIRTUAL_FUNC__;
};

class AlgGpioOut
{
public:
    virtual AlgHelperRet get_supported_count(int32_t *p_count) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet set_enabled(int32_t index, uint8_t enabled)    __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet get_enabled(int32_t index, uint8_t *p_enabled) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet set_level(int32_t index, AlgGpioLevel level) __PURE_VIRTUAL_FUNC__;
};

class AlgUart485Out
{
public:
    virtual AlgHelperRet get_supported_count(int32_t *p_count) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet set_enabled(int32_t index, uint8_t enabled)    __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet get_enabled(int32_t index, uint8_t *p_enabled) __PURE_VIRTUAL_FUNC__;
	virtual AlgHelperRet set_baudrate(int32_t index, int32_t baudrate) __PURE_VIRTUAL_FUNC__;
	virtual AlgHelperRet get_baudrate(int32_t index, int32_t *p_baudrate) __PURE_VIRTUAL_FUNC__;
    virtual AlgHelperRet write_message(int32_t index, const std::string& msg) __PURE_VIRTUAL_FUNC__;
};