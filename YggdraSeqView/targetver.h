/**
 * @file targetver.h
 * @brief 대상 Windows 플랫폼 버전 정의 (View DLL용)
 */

#pragma once

#include <winsdkver.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00     // Windows 10 이상
#endif

#include <SDKDDKVer.h>
