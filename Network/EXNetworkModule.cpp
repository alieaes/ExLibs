#include "stdafx.h"
#include "EXNetworkModule.hpp"

Ext::Network::cModuleNetwork::cModuleNetwork()
{
}

Ext::Network::cModuleNetwork::~cModuleNetwork()
{
}

bool Ext::Network::cModuleNetwork::NotifyModule( const XString& sNotifyJobs )
{
    return true;
}

bool Ext::Network::cModuleNetwork::moduleInit()
{
    return true;
}

bool Ext::Network::cModuleNetwork::moduleStart()
{
    return true;
}

bool Ext::Network::cModuleNetwork::moduleStop()
{
    return true;
}

bool Ext::Network::cModuleNetwork::moduleFinal()
{
    return true;
}
