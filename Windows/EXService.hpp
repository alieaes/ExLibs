#pragma once

#ifndef __HDR_EXT_SERVICE__
#define __HDR_EXT_SERVICE__

#include <vector>

#include "EXString.hpp"

namespace Ext
{
    namespace Service
    {
        enum class StartupType
        {
            Automatic = 0,
            AutomaticDelayedStart,
            Manual,
            Disabled
        };

        enum class RunState
        {
            Unknown = 0,
            Running,
            Stopped,
            StartPending,
            StopPending,
            Paused,
            Error           // Stopped인데 마지막 종료 코드가 정상(0)이 아닌 경우
        };

        struct ServiceInfo
        {
            XString      serviceName;
            XString      displayName;
            XString      binaryPath;
            StartupType  startupType = StartupType::Manual;
            RunState     runState    = RunState::Unknown;
            XString      remark;         // 마지막 오류 코드 등, UI의 "비고"에 대응
        };

        // SCM에 새 서비스를 등록한다 (SERVICE_WIN32_OWN_PROCESS 기준).
        bool RegisterService( const XString& serviceName, const XString& displayName, const XString& binaryPath,
                               StartupType startupType, XString& outError );

        // 실행 중이면 먼저 중지하고 SCM에서 서비스를 제거한다.
        bool DeleteServiceByName( const XString& serviceName, XString& outError );

        bool StartServiceByName( const XString& serviceName, XString& outError );
        // dwWaitMsec: 서비스가 실제로 멈추는 걸 최대 이만큼 기다린다 (0이면 요청만 보내고 바로 반환).
        bool StopServiceByName( const XString& serviceName, XString& outError, unsigned int dwWaitMsec = 5000 );
        // 중지(대기) 후 시작. 이미 멈춰있는 상태에서 호출해도 정상 동작한다.
        bool RestartService( const XString& serviceName, XString& outError );

        bool QueryService( const XString& serviceName, ServiceInfo& outInfo, XString& outError );
        // 존재하지 않거나 조회에 실패한 항목은 결과 목록에서 빠진다.
        std::vector<ServiceInfo> QueryServices( const std::vector<XString>& serviceNames );
    }
}

#endif
