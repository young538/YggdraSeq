/**
 * @file pch.h
 * @brief 미리 컴파일된 헤더 (Precompiled Header)
 *
 * 이 파일은 프로젝트의 모든 소스 파일(.cpp)에서 첫 번째로 포함되어야 합니다.
 * 자주 변경되지 않는 프레임워크 헤더와 표준 라이브러리 헤더를 미리 컴파일하여
 * 빌드 속도를 크게 향상시킵니다.
 *
 * @note Visual Studio 프로젝트 설정에서 "미리 컴파일된 헤더 사용"을 활성화하고
 *       pch.cpp의 설정은 "미리 컴파일된 헤더 만들기"로 지정해야 합니다.
 */

#pragma once

#ifndef PCH_H
#define PCH_H

// MFC/Windows 프레임워크 및 C++ 표준 라이브러리 헤더
#include "framework.h"

#endif // PCH_H
