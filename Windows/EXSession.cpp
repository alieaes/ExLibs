#include "stdafx.h"
#include "EXSession.hpp"

#include <vector>

#include <userenv.h>
#include <wtsapi32.h>

#pragma comment( lib, "Wtsapi32.lib" )
#pragma comment( lib, "Userenv.lib" )

namespace
{
    bool EnablePrivilege( LPCWSTR privilegeName, XString& outError )
    {
        HANDLE hToken = NULL;

        if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) == FALSE )
        {
            outError = "OpenProcessToken failed";
            return false;
        }

        LUID luid;
        if( LookupPrivilegeValueW( NULL, privilegeName, &luid ) == FALSE )
        {
            outError = "LookupPrivilegeValue failed";
            CloseHandle( hToken );
            return false;
        }

        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[ 0 ].Luid = luid;
        tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL isAdjusted = AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( tp ), NULL, NULL );
        DWORD dwLastError = GetLastError();

        CloseHandle( hToken );

        if( isAdjusted == FALSE || dwLastError == ERROR_NOT_ALL_ASSIGNED )
        {
            outError = "AdjustTokenPrivileges failed - service account probably lacks this privilege";
            return false;
        }

        return true;
    }
}

bool Ext::Session::GetActiveConsoleSessionId( unsigned long& outSessionId )
{
    DWORD dwSessionId = WTSGetActiveConsoleSessionId();

    if( dwSessionId == 0xFFFFFFFF )
        return false;

    outSessionId = dwSessionId;
    return true;
}

bool Ext::Session::LaunchProcessInSession( unsigned long dwSessionId, const XString& sCommandLine, const XString& sWorkingDir,
                                            unsigned long& outProcessId, XString& outError )
{
    bool isSuccess = false;

    HANDLE hUserToken    = NULL;
    HANDLE hPrimaryToken = NULL;
    LPVOID pEnvironment  = NULL;
    PROCESS_INFORMATION pi = { 0 };

    do
    {
        if( EnablePrivilege( SE_TCB_NAME, outError ) == false )
            break;

        if( EnablePrivilege( SE_ASSIGNPRIMARYTOKEN_NAME, outError ) == false )
            break;

        if( EnablePrivilege( SE_INCREASE_QUOTA_NAME, outError ) == false )
            break;

        if( WTSQueryUserToken( dwSessionId, &hUserToken ) == FALSE )
        {
            outError = "WTSQueryUserToken failed - no interactive user in that session?";
            break;
        }

        if( DuplicateTokenEx( hUserToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken ) == FALSE )
        {
            outError = "DuplicateTokenEx failed";
            break;
        }

        if( CreateEnvironmentBlock( &pEnvironment, hPrimaryToken, FALSE ) == FALSE )
        {
            outError = "CreateEnvironmentBlock failed";
            break;
        }

        STARTUPINFOW si = { 0 };
        si.cb = sizeof( si );
        si.lpDesktop = const_cast<LPWSTR>( L"winsta0\\default" );

        // CreateProcessAsUser는 명령줄 버퍼를 직접 수정할 수 있으므로 반드시 쓰기 가능한 버퍼로 넘겨야 한다.
        std::wstring sCmd = sCommandLine.toWString();
        std::vector<wchar_t> vecCmd( sCmd.begin(), sCmd.end() );
        vecCmd.push_back( L'\0' );

        LPCWSTR pWorkingDir = ( sWorkingDir.IsEmpty() == false ) ? static_cast<LPCWSTR>( sWorkingDir ) : NULL;

        BOOL isCreated = CreateProcessAsUserW(
            hPrimaryToken,
            NULL,
            vecCmd.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,
            pEnvironment,
            pWorkingDir,
            &si,
            &pi );

        if( isCreated == FALSE )
        {
            outError = "CreateProcessAsUser failed";
            break;
        }

        outProcessId = pi.dwProcessId;

        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );

        isSuccess = true;

    } while( false );

    if( pEnvironment != NULL )
        DestroyEnvironmentBlock( pEnvironment );

    if( hPrimaryToken != NULL )
        CloseHandle( hPrimaryToken );

    if( hUserToken != NULL )
        CloseHandle( hUserToken );

    return isSuccess;
}

bool Ext::Session::LaunchProcessInActiveSession( const XString& sCommandLine, const XString& sWorkingDir,
                                                  unsigned long& outProcessId, XString& outError )
{
    unsigned long dwSessionId = 0;

    if( GetActiveConsoleSessionId( dwSessionId ) == false )
    {
        outError = "No active console session (nobody logged in)";
        return false;
    }

    return LaunchProcessInSession( dwSessionId, sCommandLine, sWorkingDir, outProcessId, outError );
}
