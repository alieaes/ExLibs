#include "stdafx.h"
#include "EXService.hpp"

#include <vector>

#include <winsvc.h>

#pragma comment( lib, "advapi32.lib" )

namespace
{
    void AppendLastError( XString& outError, const char* what )
    {
        outError = what;
        outError += " (code=";
        outError += std::to_string( GetLastError() );
        outError += ")";
    }

    DWORD ToScmStartType( Ext::Service::StartupType startupType )
    {
        switch( startupType )
        {
            case Ext::Service::StartupType::Automatic:
            case Ext::Service::StartupType::AutomaticDelayedStart:
                return SERVICE_AUTO_START;
            case Ext::Service::StartupType::Manual:
                return SERVICE_DEMAND_START;
            case Ext::Service::StartupType::Disabled:
            default:
                return SERVICE_DISABLED;
        }
    }

    Ext::Service::RunState ToRunState( const SERVICE_STATUS_PROCESS& status )
    {
        switch( status.dwCurrentState )
        {
            case SERVICE_RUNNING:          return Ext::Service::RunState::Running;
            case SERVICE_START_PENDING:    return Ext::Service::RunState::StartPending;
            case SERVICE_STOP_PENDING:     return Ext::Service::RunState::StopPending;
            case SERVICE_PAUSED:           return Ext::Service::RunState::Paused;
            case SERVICE_STOPPED:
                return ( status.dwWin32ExitCode == NO_ERROR ) ? Ext::Service::RunState::Stopped : Ext::Service::RunState::Error;
            default:                        return Ext::Service::RunState::Unknown;
        }
    }

    // SC_HANDLE을 자동으로 닫아주는 RAII 래퍼
    struct ScHandle
    {
        SC_HANDLE handle = NULL;

        ScHandle() = default;
        explicit ScHandle( SC_HANDLE h ) : handle( h ) {}
        ~ScHandle() { if( handle != NULL ) CloseServiceHandle( handle ); }

        ScHandle( const ScHandle& ) = delete;
        ScHandle& operator=( const ScHandle& ) = delete;

        operator SC_HANDLE() const { return handle; }
        bool IsValid() const { return handle != NULL; }
    };
}

bool Ext::Service::RegisterService( const XString& serviceName, const XString& displayName, const XString& binaryPath,
                                     StartupType startupType, XString& outError )
{
    ScHandle hSCM( OpenSCManagerW( NULL, NULL, SC_MANAGER_CREATE_SERVICE ) );
    if( hSCM.IsValid() == false )
    {
        AppendLastError( outError, "OpenSCManager failed" );
        return false;
    }

    ScHandle hService( CreateServiceW(
        hSCM,
        serviceName,
        displayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        ToScmStartType( startupType ),
        SERVICE_ERROR_NORMAL,
        binaryPath,
        NULL, NULL, NULL, NULL, NULL ) );

    if( hService.IsValid() == false )
    {
        AppendLastError( outError, "CreateService failed" );
        return false;
    }

    if( startupType == StartupType::AutomaticDelayedStart )
    {
        SERVICE_DELAYED_AUTO_START_INFO delayedInfo = { TRUE };
        ChangeServiceConfig2W( hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo );
    }

    return true;
}

bool Ext::Service::DeleteServiceByName( const XString& serviceName, XString& outError )
{
    ScHandle hSCM( OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT ) );
    if( hSCM.IsValid() == false )
    {
        AppendLastError( outError, "OpenSCManager failed" );
        return false;
    }

    ScHandle hService( OpenServiceW( hSCM, serviceName, SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS ) );
    if( hService.IsValid() == false )
    {
        AppendLastError( outError, "OpenService failed" );
        return false;
    }

    SERVICE_STATUS status = { 0 };
    if( QueryServiceStatus( hService, &status ) == TRUE && status.dwCurrentState != SERVICE_STOPPED )
        ControlService( hService, SERVICE_CONTROL_STOP, &status );

    if( DeleteService( hService ) == FALSE )
    {
        AppendLastError( outError, "DeleteService failed" );
        return false;
    }

    return true;
}

bool Ext::Service::StartServiceByName( const XString& serviceName, XString& outError )
{
    ScHandle hSCM( OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT ) );
    if( hSCM.IsValid() == false )
    {
        AppendLastError( outError, "OpenSCManager failed" );
        return false;
    }

    ScHandle hService( OpenServiceW( hSCM, serviceName, SERVICE_START ) );
    if( hService.IsValid() == false )
    {
        AppendLastError( outError, "OpenService failed" );
        return false;
    }

    if( StartServiceW( hService, 0, NULL ) == FALSE )
    {
        AppendLastError( outError, "StartService failed" );
        return false;
    }

    return true;
}

bool Ext::Service::StopServiceByName( const XString& serviceName, XString& outError, unsigned int dwWaitMsec /*= 5000*/ )
{
    ScHandle hSCM( OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT ) );
    if( hSCM.IsValid() == false )
    {
        AppendLastError( outError, "OpenSCManager failed" );
        return false;
    }

    ScHandle hService( OpenServiceW( hSCM, serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS ) );
    if( hService.IsValid() == false )
    {
        AppendLastError( outError, "OpenService failed" );
        return false;
    }

    SERVICE_STATUS status = { 0 };

    if( ControlService( hService, SERVICE_CONTROL_STOP, &status ) == FALSE )
    {
        // 이미 멈춰있는 경우는 실패로 취급하지 않는다.
        if( GetLastError() != ERROR_SERVICE_NOT_ACTIVE )
        {
            AppendLastError( outError, "ControlService(STOP) failed" );
            return false;
        }

        return true;
    }

    DWORD dwWaited = 0;
    while( dwWaited < dwWaitMsec )
    {
        if( QueryServiceStatus( hService, &status ) == FALSE )
            break;

        if( status.dwCurrentState == SERVICE_STOPPED )
            return true;

        Sleep( 200 );
        dwWaited += 200;
    }

    return true;
}

bool Ext::Service::RestartService( const XString& serviceName, XString& outError )
{
    // 정지 실패(이미 멈춰있음 등)는 무시하고 시작을 시도한다.
    XString stopError;
    StopServiceByName( serviceName, stopError );

    return StartServiceByName( serviceName, outError );
}

bool Ext::Service::QueryService( const XString& serviceName, ServiceInfo& outInfo, XString& outError )
{
    ScHandle hSCM( OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT ) );
    if( hSCM.IsValid() == false )
    {
        AppendLastError( outError, "OpenSCManager failed" );
        return false;
    }

    ScHandle hService( OpenServiceW( hSCM, serviceName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG ) );
    if( hService.IsValid() == false )
    {
        AppendLastError( outError, "OpenService failed" );
        return false;
    }

    outInfo.serviceName = serviceName;

    SERVICE_STATUS_PROCESS statusProcess = { 0 };
    DWORD dwBytesNeeded = 0;

    if( QueryServiceStatusEx( hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>( &statusProcess ),
                               sizeof( statusProcess ), &dwBytesNeeded ) == TRUE )
    {
        outInfo.runState = ToRunState( statusProcess );

        if( outInfo.runState == RunState::Error )
        {
            outInfo.remark = "exit code=";
            outInfo.remark += std::to_string( statusProcess.dwWin32ExitCode );
        }
    }

    std::vector<BYTE> vecConfigBuf( 8192 );
    LPQUERY_SERVICE_CONFIGW pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>( vecConfigBuf.data() );

    if( QueryServiceConfigW( hService, pConfig, static_cast<DWORD>( vecConfigBuf.size() ), &dwBytesNeeded ) == TRUE )
    {
        outInfo.binaryPath = pConfig->lpBinaryPathName;
        outInfo.displayName = pConfig->lpDisplayName;

        if( pConfig->dwStartType == SERVICE_DISABLED )
        {
            outInfo.startupType = StartupType::Disabled;
        }
        else if( pConfig->dwStartType == SERVICE_DEMAND_START )
        {
            outInfo.startupType = StartupType::Manual;
        }
        else
        {
            outInfo.startupType = StartupType::Automatic;

            std::vector<BYTE> vecDelayedBuf( sizeof( SERVICE_DELAYED_AUTO_START_INFO ) + 16 );
            LPSERVICE_DELAYED_AUTO_START_INFO pDelayed = reinterpret_cast<LPSERVICE_DELAYED_AUTO_START_INFO>( vecDelayedBuf.data() );
            DWORD dwDelayedBytesNeeded = 0;

            if( QueryServiceConfig2W( hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, vecDelayedBuf.data(),
                                       static_cast<DWORD>( vecDelayedBuf.size() ), &dwDelayedBytesNeeded ) == TRUE
                && pDelayed->fDelayedAutostart != FALSE )
            {
                outInfo.startupType = StartupType::AutomaticDelayedStart;
            }
        }
    }

    return true;
}

std::vector<Ext::Service::ServiceInfo> Ext::Service::QueryServices( const std::vector<XString>& serviceNames )
{
    std::vector<ServiceInfo> results;

    for( const XString& name : serviceNames )
    {
        ServiceInfo info;
        XString error;

        if( QueryService( name, info, error ) == true )
            results.push_back( info );
    }

    return results;
}
