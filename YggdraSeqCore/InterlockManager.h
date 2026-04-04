/**
 * @file InterlockManager.h
 * @brief 전역 인터락 관리자 클래스
 *
 * 시스템 전체에서 항상 체크해야 하는 전역 인터락을 관리합니다.
 * 주기적으로 또는 전이 시 checkAll()을 호출하여 모든 인터락을 평가하고,
 * EMO 심각도의 인터락이 위반되면 EMO 콜백을 호출합니다.
 *
 * @namespace YggdraSeq
 */

#pragma once

#include "Types.h"
#include "Interlock.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace YggdraSeq
{

/**
 * @class InterlockManager
 * @brief 전역 인터락을 관리하고 체크하는 매니저 클래스
 *
 * SequenceEngine이 소유하며, 전이 시 또는 주기적으로 checkAll()을 호출합니다.
 * EMO 인터락이 위반되면 등록된 EMO 콜백을 호출하여 비상 정지를 트리거합니다.
 *
 * @note 모든 public 메서드는 스레드 안전합니다 (mutex 보호).
 */
class AFX_EXT_CLASS InterlockManager
{
public:
    /**
     * @brief InterlockManager 생성자
     */
    InterlockManager();

    /**
     * @brief 소멸자
     */
    ~InterlockManager();

    // ========================================================================
    // 인터락 등록 / 제거
    // ========================================================================

    /**
     * @brief 전역 인터락을 추가
     *
     * 추가된 인터락은 checkAll() 호출 시 항상 체크됩니다.
     * 같은 이름의 인터락이 이미 존재하면 InterlockAlreadyRegistered 에러를 반환합니다.
     *
     * @param interlock 추가할 인터락 (소유권 이전, unique_ptr)
     * @return ErrorCode 등록 결과 (Success / InterlockAlreadyRegistered)
     */
    ErrorCode addInterlock(std::unique_ptr<Interlock> interlock);

    /**
     * @brief 전역 인터락을 이름으로 제거
     *
     * @param name 제거할 인터락 이름
     * @return ErrorCode 제거 결과 (Success / InterlockNotFound)
     * @warning Transition에 바인딩된 인터락을 제거하면 dangling pointer가 됩니다.
     *          반드시 Transition에서 먼저 제거한 후 호출하세요.
     */
    ErrorCode removeInterlock(const std::string& name);

    // ========================================================================
    // 인터락 체크
    // ========================================================================

    /**
     * @brief 등록된 모든 전역 인터락을 체크
     *
     * 모든 인터락의 check() 메서드를 호출하고 결과를 수집합니다.
     * EMO 심각도의 인터락이 위반되면 EMO 콜백을 즉시 호출합니다.
     *
     * @param[out] results 각 인터락의 체크 결과 목록
     * @return true = 모든 인터락 통과, false = 하나 이상 위반
     */
    bool checkAll(std::vector<InterlockCheckResult>& results) const;

    // ========================================================================
    // EMO 콜백 설정
    // ========================================================================

    /**
     * @brief EMO 발생 시 호출될 콜백 함수를 설정
     *
     * EMO 심각도의 인터락이 위반되면 이 콜백이 호출됩니다.
     * 콜백은 위반된 인터락 정보를 인자로 받습니다.
     *
     * @param callback EMO 콜백 함수 (const Interlock& 인자)
     */
    void setEmoCallback(std::function<void(const Interlock&)> callback);

    // ========================================================================
    // 접근자
    // ========================================================================

    /**
     * @brief 이름으로 인터락을 검색
     *
     * Transition에 인터락을 바인딩할 때 이 메서드로 포인터를 얻습니다.
     *
     * @param name 검색할 인터락 이름
     * @return 인터락 포인터 (찾지 못하면 nullptr)
     * @note 반환된 포인터의 수명은 InterlockManager가 관리합니다.
     */
    Interlock* findInterlock(const std::string& name) const;

    /**
     * @brief 등록된 전역 인터락 수를 반환
     * @return 인터락 수
     */
    size_t getInterlockCount() const;

    /**
     * @brief 등록된 모든 인터락의 이름 목록을 반환
     * @return 인터락 이름 벡터
     */
    std::vector<std::string> getInterlockNames() const;

private:
    /// 전역 인터락 목록 (소유권 보유)
    std::vector<std::unique_ptr<Interlock>> m_globalInterlocks;

    /// EMO 발생 시 호출할 콜백 함수
    std::function<void(const Interlock&)> m_emoCallback;

    /// 스레드 동기화용 뮤텍스
    mutable std::mutex m_mutex;
};

} // namespace YggdraSeq
