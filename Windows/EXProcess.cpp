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

bool Ext::Process::TerminateProcessById( unsigned int uPID )
{
	bool isSuccess = false;

	HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, uPID );
	if( hProcess != NULL )
	{
		isSuccess = ( TerminateProcess( hProcess, 0 ) == TRUE );
		CloseHandle( hProcess );
	}

	return isSuccess;
}
