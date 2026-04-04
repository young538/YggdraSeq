/**
 * @file CSequenceDebugPane.h
 * @brief 시퀀스 디버그 도킹 패널
 *
 * 시퀀스 실행을 단계별로 추적하고 디버깅할 수 있는 패널입니다.
 * CDockablePane에서 파생합니다.
 *
 * 기능:
 *   - 스텝 실행 (Step Next): 시퀀스를 한 상태씩 수동 진행
 *   - 브레이크포인트: 특정 상태 진입 시 자동 일시정지
 *   - 변수 워치: 현재 상태의 파라미터 값 실시간 표시
 *   - 전이 조건 모니터: 현재 상태에서 가능한 전이의 guard/interlock 상태
 *   - 콜스택: 상태 전이 이력 (현재까지 경로)
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/Types.h"
#include <chrono>
#include <string>
#include <vector>
#include <map>

namespace YggdraSeq
{

/**
 * @class CSequenceDebugPane
 * @brief 시퀀스 디버그 도킹 패널 (CDockablePane 파생)
 *
 * 디버그 모드에서 스텝 실행, 브레이크포인트, 변수 워치를 제공합니다.
 * 엔진의 setDebugMode(), addBreakpoint(), stepNext() 등과 연동합니다.
 */
class AFX_EXT_CLASS CSequenceDebugPane : public CWnd
{
    DECLARE_DYNAMIC(CSequenceDebugPane)

public:
    CSequenceDebugPane();
    virtual ~CSequenceDebugPane();

    // ========================================================================
    // 갱신 메서드 (UI 스레드에서 호출)
    // ========================================================================

    /**
     * @brief 엔진 상태 변경 시 갱신 (디버그 패널의 Step 버튼 활성화 등)
     *
     * @param threadId  스레드 ID
     * @param newStatus 새로운 엔진 상태
     */
    void updateEngineStatus(uint32_t threadId, EngineStatus newStatus);

    /**
     * @brief 디버그 브레이크 발생 시 갱신
     *
     * 변수 워치 목록을 업데이트합니다.
     *
     * @param threadId  스레드 ID
     * @param stateName 브레이크포인트에 도달한 상태 이름
     * @param params    상태 파라미터 (key-value)
     * @param timestamp 브레이크 시간
     */
    void updateDebugBreak(uint32_t threadId, const std::string& stateName,
        const std::map<std::string, ParamValue>& params,
        const std::chrono::system_clock::time_point& timestamp);

    /**
     * @brief 전이 조건 모니터 갱신
     *
     * 현재 상태에서 가능한 전이의 guard/interlock 평가 결과를 갱신합니다.
     *
     * @param threadId         스레드 ID
     * @param fromState        출발 상태
     * @param toState          도착 상태
     * @param guardResult      guard 조건 결과
     * @param interlockResults 인터락 체크 결과 목록
     */
    void updateTransitionConditions(uint32_t threadId, const std::string& fromState,
        const std::string& toState, bool guardResult,
        const std::vector<InterlockCheckResult>& interlockResults);

    /**
     * @brief 상태 전이 발생 시 콜스택 갱신
     *
     * @param stateName 새로 진입한 상태 이름
     */
    void pushCallStack(const std::string& stateName);

    /**
     * @brief 콜스택 초기화
     */
    void clearCallStack();

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()

private:
    void initializeControls();
    void layoutControls(int cx, int cy);

    // 전이 조건 모니터 리스트
    CListCtrl m_transitionList;    ///< 전이 조건 리스트 (From->To, Guard, Interlocks)

    // 변수 워치 리스트
    CListCtrl m_watchList;         ///< 변수 워치 리스트 (Key, Value, Type)

    // 콜스택 리스트
    CListCtrl m_callStackList;     ///< 상태 전이 경로 콜스택

    // 상태 전이 경로 이력
    std::vector<std::string> m_callStack;  ///< 상태 이름 콜스택

    // 현재 엔진 상태 (Step 버튼 활성화 판단용)
    EngineStatus m_currentStatus = EngineStatus::Idle;
};

} // namespace YggdraSeq
