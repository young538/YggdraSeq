# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test

```bash
# 전체 솔루션 빌드 (VS2022 Community)
"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" YggdraSeq.sln -p:Configuration=Debug -p:Platform=x64 -p:VcpkgEnableManifest=true -m -verbosity:minimal

# 단일 프로젝트 빌드
"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" YggdraSeqCore/YggdraSeqCore.vcxproj -p:Configuration=Debug -p:Platform=x64 -p:VcpkgEnableManifest=true

# 테스트 실행
./x64/Debug/YggdraSeqTest.exe

# 앱이 실행 중이면 DLL이 잠기므로 빌드 전 종료 필요
taskkill //F //IM YggdraSeqApp.exe
```

vcpkg manifest 모드 사용. `pwsh.exe` 경고는 무시해도 됨 (PowerShell Core 미설치 시 applocal DLL 복사 스크립트 경고).

## Architecture

4개 프로젝트, 단방향 의존성:

```
YggdraSeqApp.exe ──→ YggdraSeqView.dll ──→ YggdraSeqCore.dll
YggdraSeqTest.exe ──→ YggdraSeqCore.dll
```

**YggdraSeqCore.dll** (MFC Extension DLL): 상태 머신 엔진
- `SequenceEngine` — 워커 스레드에서 상태 전이 루프 실행
- `IState` — 상태 인터페이스 (onEntry/onExecute/onExit)
- `Transition` — guard 함수 + interlock 바인딩으로 전이 조건 관리
- `InterlockManager` — 안전 조건 중앙 관리
- `ISequenceObserver` — 9개 콜백 인터페이스 (View가 구현)

**YggdraSeqView.dll** (MFC Extension DLL): 모니터링 UI
- `CSeqMonitorFrame` (CWnd) — ISequenceObserver 구현 + CMFCTabCtrl로 6개 패널 탭 관리
- 6개 패널 (CWnd 파생): CStateListPane, CInterlockPane, CLogOutputPane, CThreadMonitorPane, CSequenceDebugPane, CTransitionLogPane
- Observer 콜백(워커 스레드) → 큐에 push → PostMessage → UI 스레드 핸들러 → 패널 갱신

**YggdraSeqApp.exe**: AOI 장비 HMI 테스트 앱
- `CMainFrame` (CFrameWndEx) — layoutChildren()으로 모든 자식 수동 배치
- Inspection/Handler 듀얼 스레드 시퀀스 + SimulatedHardware

## Key Patterns

- **스레드 안전**: Observer 콜백은 워커 스레드에서 호출됨. UI 갱신은 반드시 PostMessage를 통해 UI 스레드에서 수행
- **에러 코드 기반**: 예외 미사용. ErrorCode enum으로 결과 반환
- **도킹 미사용**: View 패널은 CWnd + CMFCTabCtrl 탭 방식. CDockablePane/CFrameWndEx 사용 금지
- **DLL 익스포트**: `AFX_EXT_CLASS` 매크로 사용 (MFC Extension DLL)

## Conventions

- 네임스페이스: `YggdraSeq`
- 클래스명: PascalCase, MFC UI는 `C` 접두사, 인터페이스는 `I` 접두사
- 메서드: camelCase, 멤버변수: `m_` 접두사
- 주석: 한국어로 자세히. Doxygen 스타일 (`/** @brief */`)
- 컴파일러: `/std:c++17 /utf-8`
- 타입: `ParamValue = std::variant<int, double, std::string, bool>`
