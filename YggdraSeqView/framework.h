/**
 * @file framework.h
 * @brief MFC/Windows 프레임워크 공통 헤더 (View DLL용)
 *
 * MFC Extension DLL에서 사용하는 Windows 및 MFC 프레임워크 헤더를 포함합니다.
 * CFrameWndEx, CDockablePane, CMFCVisualManager 등 Feature Pack 헤더를 포함합니다.
 *
 * @note 자주 변경되지 않는 시스템 헤더만 이 파일에 추가하세요.
 */

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 Windows 헤더를 제외하여 빌드 속도 향상
#endif

#include "targetver.h"

// --- MFC 핵심 헤더 ---
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // CString 생성자를 명시적으로 선언
#define _AFX_ALL_WARNINGS                       // 일반적으로 무시되는 MFC 경고도 표시

#include <afxwin.h>             // MFC 코어 및 표준 컴포넌트
#include <afxext.h>             // MFC 확장 (도구 모음, 상태 표시줄 등)

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>             // MFC OLE 클래스
#include <afxodlgs.h>           // MFC OLE 대화 상자 클래스
#include <afxdisp.h>            // MFC Automation 클래스
#endif

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC Windows 공통 컨트롤 지원
#endif

#include <afxcontrolbars.h>     // MFC Feature Pack (CDockablePane, CMFCVisualManager 등)

// --- C++ 표준 라이브러리 ---
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
#include <fstream>
