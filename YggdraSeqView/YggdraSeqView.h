/**
 * @file YggdraSeqView.h
 * @brief YggdraSeqView DLL 메인 헤더
 *
 * YggdraSeqView DLL의 모든 public 헤더를 하나로 모아서 제공합니다.
 * 이 DLL을 사용하는 프로젝트(YggdraSeqApp 등)는 이 헤더 하나만 포함하면 됩니다.
 *
 * 포함 클래스:
 * - CSeqMonitorFrame: 도킹 프레임 (ISequenceObserver 구현)
 * - CStateListPane: 상태 목록 패널
 * - CInterlockPane: 인터락 패널
 * - CLogOutputPane: 로그 출력 패널
 * - CThreadMonitorPane: 스레드 모니터 패널
 * - CSequenceDebugPane: 시퀀스 디버그 패널
 * - CTransitionLogPane: 전이 이력 패널
 *
 * @namespace YggdraSeq
 */

#pragma once

// Core DLL 헤더 (인터페이스 참조)
#include "../YggdraSeqCore/YggdraSeqCore.h"

// View DLL 헤더
#include "resource.h"
#include "CSeqMonitorFrame.h"
#include "CStateListPane.h"
#include "CInterlockPane.h"
#include "CLogOutputPane.h"
#include "CThreadMonitorPane.h"
#include "CSequenceDebugPane.h"
#include "CTransitionLogPane.h"
