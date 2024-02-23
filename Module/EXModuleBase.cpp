#include "stdafx.h"
#include "EXModuleBase.hpp"

Ext::Module::cModuleBase::cModuleBase()
{
}

Ext::Module::cModuleBase::~cModuleBase()
{
    eModuleState state = GetModuleState();
    _isRunning.store( false );
    _isStop = true;

    if( state == MODULE_STATE_START )
        ModuleStop();

    if( state == MODULE_STATE_INIT || state == MODULE_STATE_STOP )
        ModuleFinal();
}

bool Ext::Module::cModuleBase::ModuleInit()
{
    bool isSuccess = false;

    do
    {
        if( _isRunning.load() == true )
            break;

        eModuleState state = GetModuleState();

        if( state == MODULE_STATE_INIT ||
            state == MODULE_STATE_START ||
            state == MODULE_STATE_STOP )
        {
            break;
        }

        _isRunning.store( true );

        isSuccess = moduleInit();
        setModuleState( MODULE_STATE_INIT );

        _isRunning.store( false );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleBase::ModuleStart()
{
    bool isSuccess = false;

    do
    {
        if( _isRunning.load() == true )
            break;

        eModuleState state = GetModuleState();

        if( state == MODULE_STATE_NONE || 
            state == MODULE_STATE_START || 
            state == MODULE_STATE_FINAL )
        {
            break;
        }

        _isRunning.store( true );

        isSuccess = moduleStart();
        setModuleState( MODULE_STATE_START );

        _isRunning.store( false );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleBase::ModuleStop()
{
    bool isSuccess = false;

    do
    {
        if( _isRunning.load() == true )
            break;

        eModuleState state = GetModuleState();

        if( state == MODULE_STATE_NONE ||
            state == MODULE_STATE_INIT ||
            state == MODULE_STATE_FINAL )
        {
            break;
        }

        _isRunning.store( true );

        isSuccess = moduleStop();
        setModuleState( MODULE_STATE_STOP );

        _isRunning.store( false );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleBase::ModuleFinal()
{
    bool isSuccess = false;

    do
    {
        if( _isRunning.load() == true )
            break;

        if( GetModuleState() != MODULE_STATE_STOP )
            break;

        _isRunning.store( true );

        isSuccess = moduleFinal();
        setModuleState( MODULE_STATE_FINAL );

        _isRunning.store( false );

    } while( false );

    return isSuccess;
}

Ext::Module::eModuleState Ext::Module::cModuleBase::GetModuleState()
{
    std::unique_lock< std::mutex > lck( _lckState );
    return _eState;
}

void Ext::Module::cModuleBase::setModuleState( eModuleState state )
{
    std::unique_lock< std::mutex > lck( _lckState );
    _eState = state;
}
