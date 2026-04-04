/**
 * @file YggdraSeqApp.h
 * @brief YggdraSeqApp MFC Application 클래스
 *
 * CWinApp에서 파생하여 MFC 애플리케이션의 진입점과 초기화를 담당합니다.
 * 다크 테마와 CMFCVisualManager를 설정합니다.
 *
 * 이 앱은 YggdraSeqCore.dll과 YggdraSeqView.dll을 참조하여
 * 반도체 외관검사 장비의 HMI 테스트 앱으로 동작합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "resource.h"

namespace YggdraSeq
{

/**
 * @class CYggdraSeqApp
 * @brief MFC Application 클래스 (CWinApp 파생)
 *
 * 애플리케이션 초기화 시:
 * - 다크 테마 비주얼 매니저 설정
 * - 레지스트리 키 설정 (도킹 레이아웃 저장용)
 * - 메인 프레임(CMainFrame) 생성
 */
class CYggdraSeqApp : public CWinApp
{
public:
    CYggdraSeqApp();
    virtual ~CYggdraSeqApp();

    /**
     * @brief 애플리케이션 초기화
     *
     * MFC 초기화, 비주얼 매니저 설정, 메인 프레임 생성을 수행합니다.
     *
     * @return TRUE = 초기화 성공
     */
    virtual BOOL InitInstance() override;

    /**
     * @brief 애플리케이션 종료
     *
     * @return 종료 코드
     */
    virtual int ExitInstance() override;

    DECLARE_MESSAGE_MAP()
};

} // namespace YggdraSeq
