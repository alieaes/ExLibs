#include "stdafx.h"
#include "EXNetwork.hpp"
#include "EXConverter.hpp"

#include "Base/ExtConfig.hpp"

#include "Util/EXUtil.hpp"

#define EXT_PACKET_SIZE 1024 * 1024 * 10

/*
 * Required Include File : EXNetwork, EXConverter, ExtConfig, EXUtil
 */


Ext::Network::tuPacketMsg Ext::Network::convertReceiveMsg(std::vector<char> vecChar)
{
    tuPacketMsg packetMsg = std::make_tuple( 0, 0, XString() );

    {
        std::vector<char> vecTmp;
        // 앞 4byte는 사이즈로 판단하기 때문에 삭제하도록 함.
        for( int idx = 0; idx < 4; idx++ )
        {
            vecTmp.push_back( vecChar.at( 0 ) );
            vecChar.erase( vecChar.begin() + 0 );
        }

        unsigned int nSize = vecTmp[ 0 ];
        nSize = ( nSize << 8 ) | vecTmp[ 1 ];
        nSize = ( nSize << 8 ) | vecTmp[ 2 ];
        nSize = ( nSize << 8 ) | vecTmp[ 3 ];

        std::get<0>( packetMsg ) = nSize;
    }

    {
        std::vector<char> vecTmp;
        // 그 뒤 4byte는 msgID로 관리하기 때문에 동일하게 삭제하도록 함.
        for( int idx = 0; idx < 4; idx++ )
        {
            vecTmp.push_back( vecChar.at( 0 ) );
            vecChar.erase( vecChar.begin() + 0 );
        }

        unsigned int nSize = vecTmp[ 0 ];
        nSize = ( nSize << 8 ) | vecTmp[ 1 ];
        nSize = ( nSize << 8 ) | vecTmp[ 2 ];
        nSize = ( nSize << 8 ) | vecTmp[ 3 ];

        std::get<1>( packetMsg ) = nSize;
    }

    std::get<2>( packetMsg ) = Convert::s2ws( vecChar.data() );

    return packetMsg;
}

Ext::Network::cNetwork::cNetwork()
{
}

Ext::Network::cNetwork::cNetwork( NETWORK_INFO info )
{
}

Ext::Network::cNetwork::~cNetwork()
{
    if( _thServerListen.joinable() == true )
        _thServerListen.join();
}

void Ext::Network::cNetwork::SetGlobalServerHandler( fnHandler fn, bool isNeedUpdate /*= false*/ )
{
    _fnServerHandler = fn;

    if( isNeedUpdate == true )
    {
        
    }
}

bool Ext::Network::cNetwork::Connect()
{
    bool isConnected = false;

    do
    {
        if( _info.isValid() == false )
            break;

        if( _info.eType == NETWORK_CLIENT )
        {
            if( _spTCPClient == NULLPTR )
                _spTCPClient = std::make_shared<CTCPClient>( std::bind( &cNetwork::networkLogging, this, std::placeholders::_1 ) );

            if( _spTCPClient->Connect( _info.sIP, std::to_string( _info.dwPort ) ) == false )
            {
                break;
            }
        }
        else if( _info.eType == NETWORK_SERVER )
        {
            if( _spTCPServer == NULLPTR )
                _spTCPServer = std::make_shared<CTCPServer>( std::bind( &cNetwork::networkLogging, this, std::placeholders::_1 ), std::to_string( _info.dwPort ) );

            _thServerListen = std::thread( [this] { serverListenThread(); } );
        }

    } while( false );

    return isConnected;
}

bool Ext::Network::cNetwork::DisConnect()
{
    if( _info.eType != NETWORK_CLIENT )
    {
        assert( false );
        return false;
    }

    return _spTCPClient->Disconnect();
}

bool Ext::Network::cNetwork::DisConnect( const XString& sUUID )
{
    if( _info.eType != NETWORK_SERVER )
    {
        assert( false );
        return false;
    }

    if( _mapUUIDToClientInfo.contains( sUUID ) == true )
    {
        _mapUUIDToClientInfo[ sUUID ].spServerReceive.get()->Stop();
        _mapUUIDToClientInfo.erase( sUUID );
        return _spTCPServer->Disconnect( _mapUUIDToClientInfo[ sUUID ].connectionClient );
    }
    return false;
}

bool Ext::Network::cNetwork::DisConnectAll()
{
}

void Ext::Network::cNetwork::networkLogging( const std::string& sLog )
{
    
}

void Ext::Network::cNetwork::serverListenThread()
{
    while( _isStop == false )
    {
        ASocket::Socket ConnectedClient;
        bool isNewConnect = _spTCPServer->Listen( ConnectedClient );

        if( isNewConnect == true )
        {
            NETWORK_CLIENT_INFO info;

            XString sUUID = Util::CreateGUID( CASE_TYPE_UPPER );

            if( _mapUUIDToClientInfo.contains( sUUID ) == true )
                sUUID = Util::CreateGUID( CASE_TYPE_UPPER );

            info.sUUID = sUUID;
            info.connectionClient = ConnectedClient;
            info.spServerReceive = std::make_shared< cNetworkServerReceive >();
            info.pNetwork = this;

            if( _fnServerHandler != NULLPTR )
                info.fnServerHandler = _fnServerHandler;

            info.spServerReceive.get()->SetClientInfo( info );

            _mapUUIDToClientInfo.insert( std::make_pair( sUUID, info ) );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

Ext::Network::cNetworkServerReceive::cNetworkServerReceive()
{
}

Ext::Network::cNetworkServerReceive::cNetworkServerReceive( NETWORK_CLIENT_INFO info )
{
    SetClientInfo( std::move( info ) );
}

Ext::Network::cNetworkServerReceive::~cNetworkServerReceive()
{
    Stop();
}

void Ext::Network::cNetworkServerReceive::SetClientInfo( NETWORK_CLIENT_INFO info )
{
    _clientInfo = std::move( info );
}

void Ext::Network::cNetworkServerReceive::Start()
{
    Stop();

    _isStop = false;

    _spHandler = std::make_shared< cNetworkServerReceiveHandler >();

    _spHandler.get()->SetClientSocket( _clientInfo.connectionClient );
    _spHandler.get()->SetUUID( _clientInfo.sUUID );

    if( _clientInfo.fnServerHandler != NULLPTR )
        _spHandler.get()->SetHandler( _clientInfo.fnServerHandler );

    _thServerReceive = std::thread( [this] { receiveThread(); } );
}

void Ext::Network::cNetworkServerReceive::Stop()
{
    _isStop = true;

    if( _thServerReceive.joinable() == true )
        _thServerReceive.join();

    _spHandler.reset();
}

void Ext::Network::cNetworkServerReceive::receiveThread()
{
    while( _isStop == false )
    {
        std::vector<char> vecRcvBuf( EXT_PACKET_SIZE );
        int nRcvBytes = _clientInfo.pNetwork->_spTCPServer->Receive( _clientInfo.connectionClient, vecRcvBuf.data(), EXT_PACKET_SIZE );

        if( nRcvBytes > 0 )
        {
            // Success

            auto tuPacket = convertReceiveMsg( vecRcvBuf );

            _spHandler->InsertPacket( tuPacket );
        }
        else if( nRcvBytes == 0 )
        {
            // Connection Close

            _clientInfo.spServerReceive.get()->Stop();
            _clientInfo.pNetwork->DisConnect( _clientInfo.sUUID );
            break;
        }
        else
        {
            // Falied..
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

Ext::Network::cNetworkServerReceiveHandler::cNetworkServerReceiveHandler()
{
}

Ext::Network::cNetworkServerReceiveHandler::~cNetworkServerReceiveHandler()
{
    _isStop = true;
    _eventQueue.Stop();
}

void Ext::Network::cNetworkServerReceiveHandler::InsertPacket( tuPacketMsg tuMsg )
{
    _eventQueue.EnQueue( tuMsg );
}

void Ext::Network::cNetworkServerReceiveHandler::serverListenThread()
{
    while( _isStop == false )
    {
        tuPacketMsg tuPacket = _eventQueue.DeQueue();

        if( std::get<0>( tuPacket ) == 0 )
        {
            Sleep( 5 );
            continue;
        }

        if( _fnHandler != NULLPTR )
            _fnHandler( _connection, tuPacket );
    }
}

//////////////////////////////////////////////////////////////////////////////
