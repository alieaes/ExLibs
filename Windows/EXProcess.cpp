#include "stdafx.h"

#include "EXProcess.hpp"

bool Ext::Process::IsRunningProcess( unsigned uPID )
{
	bool isRunning = false;

	do
	{
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, uPID );
		if( hProcess == NULL )
			break;

		DWORD dwExitCode = 0;

		if( GetExitCodeProcess( hProcess, &dwExitCode ) == TRUE )
			isRunning = ( dwExitCode == STILL_ACTIVE );

		CloseHandle( hProcess );

	} while( false );

	return isRunning;
}
