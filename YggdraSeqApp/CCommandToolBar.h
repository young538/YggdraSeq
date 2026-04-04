/**
 * @file CCommandToolBar.h
 * @brief 상단 명령 툴바
 *
 * CMFCToolBar에서 파생하여 시퀀스 제어 버튼, 레시피 선택,
 * 운전 모드 전환, LED 인디케이터를 제공합니다.
 *
 * 구성:
 *   [Run] [Pause] [Stop] [EMO]  |  Recipe: [combo]  |  Mode: [Auto/Manual/Debug]  |  LED
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"
#include "../YggdraSeqCore/Types.h"

namespace YggdraSeq
{

/**
 * @enum RunMode
 * @brief 시퀀스 운전 모드
 */
enum class RunMode
{
    Auto,       ///< 자동 실행 (전체 시퀀스 자동 진행)
    Manual,     ///< 수동 모드 (스텝별 수동 진행)
    Debug       ///< 디버그 모드 (브레이크포인트 활성)
};

/**
 * @class CCommandToolBar
 * @brief 상단 명령 툴바 (CMFCToolBar 파생)
 *
 * 시퀀스 제어 버튼과 상태 표시를 포함합니다.
 * 메인 프레임의 OnCommand에서 각 버튼 ID에 대한 처리를 구현합니다.
 */
class CCommandToolBar : public CMFCToolBar
{
    DECLARE_DYNAMIC(CCommandToolBar)

public:
    CCommandToolBar();
    virtual ~CCommandToolBar();

    /**
     * @brief 툴바 생성 및 버튼 배치
     *
     * @param pParentWnd 부모 윈도우
     * @return TRUE = 성공
     */
    BOOL CreateToolBar(CWnd* pParentWnd);

    /**
     * @brief 엔진 상태에 따라 LED 인디케이터 색상 갱신
     *
     * @param status 엔진 상태
     */
    void updateLedIndicator(EngineStatus status);

    /**
     * @brief 현재 운전 모드 반환
     * @return 현재 RunMode
     */
    RunMode getCurrentMode() const { return m_currentMode; }

    /**
     * @brief 운전 모드 설정
     * @param mode 새로운 모드
     */
    void setRunMode(RunMode mode) { m_currentMode = mode; }

protected:
    DECLARE_MESSAGE_MAP()

private:
    RunMode m_currentMode;          ///< 현재 운전 모드
    EngineStatus m_currentStatus;   ///< 현재 엔진 상태 (LED용)
};

} // namespace YggdraSeq
