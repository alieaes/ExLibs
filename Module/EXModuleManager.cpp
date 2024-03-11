#include "stdafx.h"
#include "EXModuleManager.hpp"

Ext::Module::cModuleManager::cModuleManager()
{
    _thNotify = std::thread( [this] { notifyAllModule(); } );
}

Ext::Module::cModuleManager::~cModuleManager()
{
    _isStop = true;

    if( _thNotify.joinable() == true )
        _thNotify.join();
}

bool Ext::Module::cModuleManager::RegisterModule( const XString& sModuleName, const XString& sModuleGroup, cModuleBase* cModule )
{
    bool isSuccess = false;

    do
    {
        if( IsExistModule( sModuleName ) == true )
            break;

        const auto& spModule = std::shared_ptr< cModuleBase >( cModule );

        isSuccess = insertModule( sModuleName, sModuleGroup, spModule );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::RegisterModule( const XString& sModuleName, const XString& sModuleGroup, spModuleBase spModule )
{
    bool isSuccess = false;

    do
    {
        if( IsExistModule( sModuleName ) == true )
            break;

        isSuccess = insertModule( sModuleName, sModuleGroup, spModule );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::UnRegisterModule( const XString& sModuleName )
{
    bool isSuccess = false;

    do
    {
        if( IsExistModule( sModuleName ) == false )
        {
            isSuccess = true;
            break;
        }

        isSuccess = deleteModule( sModuleName );

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::NotifyAllModule( const XString& sNotifyJobs )
{
    bool isSuccess = false;

    do
    {
        const MODULE_NOTIFY_INFO info( sNotifyJobs );

        _notifyQueue.EnQueue( info );

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::NotifyGroupModule( const XString& sGroup, const XString& sNotifyJobs )
{
    bool isSuccess = false;

    do
    {
        const MODULE_NOTIFY_INFO info( sGroup, sNotifyJobs );

        _notifyQueue.EnQueue( sNotifyJobs );

        isSuccess = true;

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::IsExistModule( const XString& sModuleName )
{
    bool isSuccess = false;

    do
    {
        std::shared_lock< std::shared_mutex > lck( _lckModule );

        if( _mapModuleNameToModule.contains( sModuleName ) == false )
            break;

        isSuccess = true;

    } while ( false );

    return isSuccess;
}

//////////////////////////////////////////////////////////////////////////////

Ext::Module::spModuleBase Ext::Module::cModuleManager::GetModuleBase( const XString& sModuleName )
{
    spModuleBase module = NULLPTR;

    do
    {
        std::shared_lock< std::shared_mutex > lck( _lckModule );

        if( IsExistModule( sModuleName ) == false )
            break;

        module = _mapModuleNameToModule[ sModuleName ];
    }
    while ( false );

    return module;
}

bool Ext::Module::cModuleManager::insertModule( const XString& sModuleName, const XString& sModuleGroup, const spModuleBase& spModule )
{
    bool isSuccess = false;

    do
    {
        std::lock_guard< std::shared_mutex > lck( _lckModule );

        _mapModuleNameToModule[ sModuleName ] = spModule;

        spModule->_sModuleName = sModuleName;
        spModule->_sModuleGroup = sModuleGroup;

        isSuccess = spModule->ModuleInit();

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::insertModule( const XString& sModuleName, const spModuleBase& spModule )
{
    bool isSuccess = false;

    do
    {
        std::lock_guard< std::shared_mutex > lck( _lckModule );

        _mapModuleNameToModule[ sModuleName ] = spModule;

        isSuccess = spModule->ModuleInit();

    } while( false );

    return isSuccess;
}

bool Ext::Module::cModuleManager::deleteModule( const XString& sModuleName )
{
    bool isSuccess = false;

    do
    {
        std::lock_guard< std::shared_mutex > lck( _lckModule );

        const spModuleBase spModule = GetModuleBase( sModuleName );

        isSuccess = spModule->ModuleStop();

        if( isSuccess == false )
            break;

        isSuccess = spModule->ModuleFinal();

        if( isSuccess == false )
            break;

        _mapModuleNameToModule.erase( sModuleName );

    } while( false );

    return isSuccess;
}

void Ext::Module::cModuleManager::notifyAllModule()
{
    while( _isStop == false )
    {
        const MODULE_NOTIFY_INFO info = _notifyQueue.DeQueue();

        if( info.isEmpty() == true )
        {
            Sleep( 100 );
            continue;
        }

        for( const auto& [key, value] : _mapModuleNameToModule )
        {
            std::shared_lock< std::shared_mutex > lck( _lckModule );

            if( value->GetModuleState() != MODULE_STATE_START )
                continue;

            if( info.sGroup.IsEmpty() == false )
            {
                if( value->GetModuleGroup() != info.sGroup )
                    continue;
            }

            value->NotifyModule( info.sJobs );
        }

        Sleep( 100 );
    }
}
