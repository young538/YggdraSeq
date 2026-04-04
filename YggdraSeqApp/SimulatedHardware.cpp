/**
 * @file SimulatedHardware.cpp
 * @brief 가상 외관검사 하드웨어 시뮬레이터 구현
 */

#include "pch.h"
#include "SimulatedHardware.h"
#include "resource.h"
#include <cmath>

namespace YggdraSeq
{

SimulatedHardware::SimulatedHardware()
    : m_cameraFocus(0.0)
    , m_lightIntensity(0.0)
    , m_stageX(0.0)
    , m_stageY(0.0)
    , m_stageTheta(0.0)
    , m_armX(300.0)     // 안전 위치 (검사 영역 밖)
    , m_armY(0.0)
    , m_gripState(false)
    , m_waferCount(25)
    , m_goodCount(0)
    , m_ngCount(0)
    , m_waferOnStage(false)
    , m_hNotifyWnd(nullptr)
{
}

SimulatedHardware::~SimulatedHardware()
{
}

// ============================================================================
// 카메라
// ============================================================================

void SimulatedHardware::setCameraFocus(double focus)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cameraFocus = focus;
    notifyUpdate();
}

double SimulatedHardware::getCameraFocus() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cameraFocus;
}

// ============================================================================
// 조명
// ============================================================================

void SimulatedHardware::setLightIntensity(double intensity)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lightIntensity = (std::max)(0.0, (std::min)(100.0, intensity));
    notifyUpdate();
}

double SimulatedHardware::getLightIntensity() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lightIntensity;
}

// ============================================================================
// 스테이지
// ============================================================================

void SimulatedHardware::setStageX(double x)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stageX = x;
    notifyUpdate();
}

double SimulatedHardware::getStageX() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stageX;
}

void SimulatedHardware::setStageY(double y)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stageY = y;
    notifyUpdate();
}

double SimulatedHardware::getStageY() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stageY;
}

void SimulatedHardware::setStageTheta(double theta)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stageTheta = theta;
    notifyUpdate();
}

double SimulatedHardware::getStageTheta() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stageTheta;
}

// ============================================================================
// 로봇 팔
// ============================================================================

void SimulatedHardware::setArmX(double x)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_armX = x;
    notifyUpdate();
}

double SimulatedHardware::getArmX() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_armX;
}

void SimulatedHardware::setArmY(double y)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_armY = y;
    notifyUpdate();
}

double SimulatedHardware::getArmY() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_armY;
}

void SimulatedHardware::setGripState(bool gripping)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gripState = gripping;
    notifyUpdate();
}

bool SimulatedHardware::getGripState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_gripState;
}

// ============================================================================
// 웨이퍼 카운터
// ============================================================================

void SimulatedHardware::setWaferCount(int count)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_waferCount = (std::max)(0, count);
    notifyUpdate();
}

int SimulatedHardware::getWaferCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_waferCount;
}

void SimulatedHardware::incrementGoodCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_goodCount++;
    notifyUpdate();
}

int SimulatedHardware::getGoodCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_goodCount;
}

void SimulatedHardware::incrementNgCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ngCount++;
    notifyUpdate();
}

int SimulatedHardware::getNgCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ngCount;
}

// ============================================================================
// 초기화
// ============================================================================

void SimulatedHardware::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cameraFocus = 0.0;
    m_lightIntensity = 0.0;
    m_stageX = 0.0;
    m_stageY = 0.0;
    m_stageTheta = 0.0;
    m_armX = 300.0;
    m_armY = 0.0;
    m_gripState = false;
    m_waferCount = 25;
    m_goodCount = 0;
    m_ngCount = 0;
    m_waferOnStage.store(false);
    notifyUpdate();
}

// ============================================================================
// 인터락 조건 헬퍼
// ============================================================================

bool SimulatedHardware::isArmOutOfInspectArea() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 검사 영역은 X < 200mm. 로봇 팔이 200mm 이상이면 안전
    return m_armX >= 200.0;
}

bool SimulatedHardware::isWaferPresent() const
{
    return m_waferOnStage.load();
}

bool SimulatedHardware::isStageStable() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 목표 위치 (125, 80, 0) 근처에 있으면 안정
    constexpr double kTolerance = 1.0;
    return std::abs(m_stageX - 125.0) < kTolerance &&
           std::abs(m_stageY - 80.0) < kTolerance &&
           std::abs(m_stageTheta) < kTolerance;
}

// ============================================================================
// UI 통지
// ============================================================================

void SimulatedHardware::setNotifyWindow(HWND hWnd)
{
    m_hNotifyWnd = hWnd;
}

void SimulatedHardware::notifyUpdate()
{
    if (m_hNotifyWnd != nullptr && ::IsWindow(m_hNotifyWnd))
    {
        ::PostMessage(m_hNotifyWnd, WM_APP_HARDWARE_UPDATED, 0, 0);
    }
}

} // namespace YggdraSeq
