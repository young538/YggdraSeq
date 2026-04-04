/**
 * @file YggdraSeqCore.h
 * @brief YggdraSeqCore DLL 메인 헤더
 *
 * 이 파일을 포함하면 YggdraSeqCore DLL의 모든 public 클래스에 접근할 수 있습니다.
 * 외부 프로젝트(View DLL, App, Test)에서 Core DLL을 사용할 때 이 파일만 포함하면 됩니다.
 *
 * 포함되는 클래스:
 * - Types (공통 열거형, 에러 코드, 타입 정의)
 * - IState (상태 인터페이스)
 * - Transition (전이 클래스)
 * - Interlock (인터락 클래스)
 * - InterlockManager (인터락 매니저)
 * - ISequenceObserver (Observer 인터페이스)
 * - SequenceEngine (메인 시퀀스 엔진)
 * - Logger (spdlog 래퍼)
 *
 * @note AFX_EXT_CLASS 매크로는 MFC Extension DLL에서 클래스를 내보내기/가져오기 합니다.
 *       DLL 빌드 시 __declspec(dllexport), DLL 사용 시 __declspec(dllimport)로 동작합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

// === 공통 타입 ===
#include "Types.h"

// === 인터페이스 ===
#include "IState.h"
#include "ISequenceObserver.h"

// === 핵심 클래스 ===
#include "Transition.h"
#include "Interlock.h"
#include "InterlockManager.h"
#include "SequenceEngine.h"
#include "Logger.h"
