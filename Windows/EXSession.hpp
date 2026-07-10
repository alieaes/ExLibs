#pragma once

#ifndef __HDR_EXT_SESSION__
#define __HDR_EXT_SESSION__

#include "EXString.hpp"

namespace Ext
{
    namespace Session
    {
        // 현재 로그인해서 활성 상태인 콘솔 세션의 ID를 찾는다. 로그인한 사용자가 없으면 false.
        bool GetActiveConsoleSessionId( unsigned long& outSessionId );

        // SYSTEM 권한(서비스)에서 호출: 지정한 세션의 사용자 토큰을 얻어와 그 세션의
        // 대화형 데스크톱(winsta0\default)에 프로세스를 띄운다. UI가 있는 프로그램도 화면에 보인다.
        // 호출하는 서비스 계정에 SeTcbPrivilege / SeAssignPrimaryTokenPrivilege / SeIncreaseQuotaPrivilege가
        // 있어야 하며(LocalSystem은 기본적으로 보유), 내부에서 필요한 만큼 활성화한다.
        bool LaunchProcessInSession( unsigned long dwSessionId, const XString& sCommandLine, const XString& sWorkingDir,
                                     unsigned long& outProcessId, XString& outError );

        // 현재 활성 콘솔 세션에 프로세스를 띄운다 (GetActiveConsoleSessionId + LaunchProcessInSession 조합)
        bool LaunchProcessInActiveSession( const XString& sCommandLine, const XString& sWorkingDir,
                                           unsigned long& outProcessId, XString& outError );
    }
}

#endif
