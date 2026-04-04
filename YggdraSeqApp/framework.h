/**
 * @file framework.h
 * @brief MFC/Windows 프레임워크 공통 헤더 (App용)
 *
 * MFC Application에서 사용하는 Windows 및 MFC 프레임워크 헤더를 포함합니다.
 * CFrameWndEx, CDockablePane, CMFCVisualManager 등 Feature Pack 헤더를 포함합니다.
 */

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _AFX_ALL_WARNINGS

#include <afxwin.h>
#include <afxext.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>
#include <afxodlgs.h>
#include <afxdisp.h>
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

#include <afxcontrolbars.h>

// C++ 표준 라이브러리
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <optional>
#include <variant>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <deque>
#include <sstream>
#include <random>
#include <cmath>
