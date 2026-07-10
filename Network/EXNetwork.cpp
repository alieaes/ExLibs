#include "stdafx.h"
#include "EXNetwork.hpp"

#include <cassert>

#include "Base/ExtConfig.hpp"

#include "Util/EXUtil.hpp"

#define EXT_PACKET_SIZE 1024 * 1024 * 10

/*
 * Required Include File : EXNetwork, EXConverter, ExtConfig, EXUtil
 */


Ext::Network::tuPacketMsg Ext::Network::convertReceiveMsg(std::vector<char> vecChar)
{
    tuPacketMsg packetMsg = std::make_tuple( 0, 0, PacketData() );

    unsigned int nSize = 0;

    {
        std::vector<char> vecTmp;
        // 앞 4byte는 사이즈로 판단하기 때문에 삭제하도록 함.
        for( int idx = 0; idx < 4; idx++ )
        {
            vecTmp.push_back( vecChar.at( 0 ) );
            vecChar.erase( vecChar.begin() + 0 );
        }

        nSize = static_cast<unsigned char>( vecTmp[ 0 ] );
        nSize = ( nSize << 8 ) | static_cast<unsigned char>( vecTmp[ 1 ] );
        nSize = ( nSize << 8 ) | static_cast<unsigned char>( vecTmp[ 2 ] );
        nSize = ( nSize << 8 ) | static_cast<unsigned char>( vecTmp[ 3 ] );

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

        unsigned int nMsgId = static_cast<unsigned char>( vecTmp[ 0 ] );
        nMsgId = ( nMsgId << 8 ) | static_cast<unsigned char>( vecTmp[ 1 ] );
        nMsgId = ( nMsgId << 8 ) | static_cast<unsigned char>( vecTmp[ 2 ] );
        nMsgId = ( nMsgId << 8 ) | static_cast<unsigned char>( vecTmp[ 3 ] );

        std::get<1>( packetMsg ) = nMsgId;
    }

    // 수신 버퍼는 EXT_PACKET_SIZE만큼 미리 할당되어 있으므로, 실제로 보낸 크기(nSize)만큼만 잘라서 페이로드로 사용한다.
    nSize = ( std::min )( nSize, static_cast<unsigned int>( vecChar.size() ) );
    vecChar.resize( nSize );

    std::get<2>( packetMsg ) = std::move( vecChar );

    return packetMsg;
}

std::vector<char> Ext::Network::convertSendMsg( MSGID msgId, const PacketData& vecData )
{
    unsigned int nSize = static_cast<unsigned int>( vecData.size() );

    std::vector<char> vecPacket;
    vecPacket.reserve( 8 + vecData.size() );

    vecPacket.push_back( static_cast<char>( ( nSize >> 24 ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>( ( nSize >> 16 ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>( ( nSize >> 8  ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>(   nSize         & 0xFF ) );

    vecPacket.push_back( static_cast<char>( ( msgId >> 24 ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>( ( msgId >> 16 ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>( ( msgId >> 8  ) & 0xFF ) );
    vecPacket.push_back( static_cast<char>(   msgId         & 0xFF ) );

    vecPacket.insert( vecPacket.end(), vecData.begin(), vecData.end() );

    return vecPacket;
}

Ext::Network::cNetwork::cNetwork()
{
}

Ext::Network::cNetwork::cNetwork( NETWORK_INFO info )
{
    _info = info;
}

Ext::Network::cNetwork::~cNetwork()
{
    _isStop = true;

    if( _info.eType == NETWORK_SERVER )
        DisConnectAll();
    else if( _info.eType == NETWORK_CLIENT )
        DisConnect();

    if( _thServerListen.joinable() == true )
        _thServerListen.join();

    if( _thClientReceive.joinable() == true )
        _thClientReceive.join();
}

void Ext::Network::cNetwork::SetGlobalServerHandler( fnHandler fn, bool isNeedUpdate /*= false*/ )
{
    _fnServerHandler = fn;

    if( isNeedUpdate == true )
    {

    }
}

void Ext::Network::cNetwork::SetGlobalClientHandler( fnHandler fn )
{
    _fnClientHandler = fn;
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

            _thClientReceive = std::thread( [this] { clientReceiveThread(); } );
        }
        else if( _info.eType == NETWORK_SERVER )
        {
            if( _spTCPServer == NULLPTR )
                _spTCPServer = std::make_shared<CTCPServer>( std::bind( &cNetwork::networkLogging, this, std::placeholders::_1 ), std::to_string( _info.dwPort ) );

            _thServerListen = std::thread( [this] { serverListenThread(); } );
        }

        isConnected = true;

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

    if( _spTCPClient == NULLPTR )
        return false;

    return _spTCPClient->Disconnect();
}

bool Ext::Network::cNetwork::DisConnect( const XString& sUUID )
{
    if( _info.eType != NETWORK_SERVER )
    {
        assert( false );
        return false;
    }

    std::lock_guard< std::mutex > lck( _lckClientMap );

    if( _mapUUIDToClientInfo.contains( sUUID ) == true )
    {
        ASocket::Socket connection = _mapUUIDToClientInfo[ sUUID ].connectionClient;

        _mapUUIDToClientInfo[ sUUID ].spServerReceive.get()->Stop();
        _mapUUIDToClientInfo.erase( sUUID );
        return _spTCPServer->Disconnect( connection );
    }
    return false;
}

bool Ext::Network::cNetwork::DisConnectAll()
{
    if( _info.eType != NETWORK_SERVER )
    {
        assert( false );
        return false;
    }

    std::lock_guard< std::mutex > lck( _lckClientMap );

    bool isSuccess = true;

    for( auto& pair : _mapUUIDToClientInfo )
    {
        pair.second.spServerReceive.get()->Stop();
        isSuccess &= _spTCPServer->Disconnect( pair.second.connectionClient );
    }

    _mapUUIDToClientInfo.clear();

    return isSuccess;
}

bool Ext::Network::cNetwork::SendToClient( const XString& sUUID, const PacketData& vecData )
{
    if( _info.eType != NETWORK_SERVER )
    {
        assert( false );
        return false;
    }

    std::lock_guard< std::mutex > lck( _lckClientMap );

    if( _mapUUIDToClientInfo.contains( sUUID ) == false )
        return false;

    MSGID msgId = _msgIdSeq++;
    std::vector<char> vecPacket = convertSendMsg( msgId, vecData );

    return _spTCPServer->Send( _mapUUIDToClientInfo[ sUUID ].connectionClient, vecPacket );
}

bool Ext::Network::cNetwork::SendToAll( const PacketData& vecData )
{
    if( _info.eType != NETWORK_SERVER )
    {
        assert( false );
        return false;
    }

    std::lock_guard< std::mutex > lck( _lckClientMap );

    MSGID msgId = _msgIdSeq++;
    std::vector<char> vecPacket = convertSendMsg( msgId, vecData );

    bool isSuccess = true;

    for( auto& pair : _mapUUIDToClientInfo )
        isSuccess &= _spTCPServer->Send( pair.second.connectionClient, vecPacket );

    return isSuccess;
}

Ext::Network::MSGID Ext::Network::cNetwork::Send( const PacketData& vecData )
{
    if( _info.eType != NETWORK_CLIENT )
    {
        assert( false );
        return 0;
    }

    if( _spTCPClient == NULLPTR )
        return 0;

    MSGID msgId = _msgIdSeq++;
    std::vector<char> vecPacket = convertSendMsg( msgId, vecData );

    if( _spTCPClient->Send( vecPacket ) == false )
        return 0;

    return msgId;
}

bool Ext::Network::cNetwork::SendAndWait( const PacketData& vecData, tuPacketMsg& outResponse, unsigned int msTimeout /*= 5000*/ )
{
    if( _info.eType != NETWORK_CLIENT )
    {
        assert( false );
        return false;
    }

    if( _spTCPClient == NULLPTR )
        return false;

    MSGID msgId = _msgIdSeq++;

    std::future< tuPacketMsg > future;

    {
        std::lock_guard< std::mutex > lck( _lckPendingRequests );

        std::promise< tuPacketMsg > promise;
        future = promise.get_future();

        _mapPendingRequests.emplace( msgId, std::move( promise ) );
    }

    std::vector<char> vecPacket = convertSendMsg( msgId, vecData );

    if( _spTCPClient->Send( vecPacket ) == false )
    {
        std::lock_guard< std::mutex > lck( _lckPendingRequests );
        _mapPendingRequests.erase( msgId );
        return false;
    }

    bool isSuccess = false;

    if( future.wait_for( std::chrono::milliseconds( msTimeout ) ) == std::future_status::ready )
    {
        outResponse = future.get();
        isSuccess = true;
    }
    else
    {
        std::lock_guard< std::mutex > lck( _lckPendingRequests );
        _mapPendingRequests.erase( msgId );
    }

    return isSuccess;
}

void Ext::Network::cNetwork::networkLogging( const std::string& sLog )
{
    
}

void Ext::Network::cNetwork::serverListenThread()
{
    while( _isStop == false )
    {
        ASocket::Socket ConnectedClient;
        bool isNewConnect = _spTCPServer->Listen( ConnectedClient, NETWORK_ACCEPT_POLL_MSEC );

        if( isNewConnect == true )
        {
            NETWORK_CLIENT_INFO info;

            std::lock_guard< std::mutex > lck( _lckClientMap );

            XString sUUID = Util::CreateGUID( CASE_TYPE_UPPER );

            while( _mapUUIDToClientInfo.contains( sUUID ) == true )
                sUUID = Util::CreateGUID( CASE_TYPE_UPPER );

            info.sUUID = sUUID;
            info.connectionClient = ConnectedClient;
            info.spServerReceive = std::make_shared< cNetworkServerReceive >();
            info.pNetwork = this;

            if( _fnServerHandler != NULLPTR )
                info.fnServerHandler = _fnServerHandler;

            // 리시버에게 shared_ptr 전달 금지. 자기 참조 사이클 생성으로 연결 해제 후에도 살아 있게 됨.
            NETWORK_CLIENT_INFO infoForReceiver = info;
            infoForReceiver.spServerReceive.reset();

            info.spServerReceive.get()->SetClientInfo( infoForReceiver );

            _mapUUIDToClientInfo.insert( std::make_pair( sUUID, info ) );

            _mapUUIDToClientInfo[ sUUID ].spServerReceive.get()->Start();
        }
    }
}

void Ext::Network::cNetwork::clientReceiveThread()
{
    while( _isStop == false )
    {
        std::vector<char> vecRcvBuf( EXT_PACKET_SIZE );
        int nRcvBytes = _spTCPClient->Receive( vecRcvBuf.data(), EXT_PACKET_SIZE );

        if( nRcvBytes > 0 )
        {
            auto tuPacket = convertReceiveMsg( vecRcvBuf );
            MSGID msgId = std::get<1>( tuPacket );

            std::promise< tuPacketMsg > pendingPromise;
            bool isPending = false;

            {
                std::lock_guard< std::mutex > lck( _lckPendingRequests );

                auto it = _mapPendingRequests.find( msgId );
                if( it != _mapPendingRequests.end() )
                {
                    pendingPromise = std::move( it->second );
                    _mapPendingRequests.erase( it );
                    isPending = true;
                }
            }

            if( isPending == true )
                pendingPromise.set_value( tuPacket );
            else if( _fnClientHandler != NULLPTR )
                _fnClientHandler( _spTCPClient->GetSocketDescriptor(), tuPacket );
        }
        else if( nRcvBytes == 0 )
        {
            // Connection Close
            break;
        }
        else
        {
            // Failed..
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
    {
        // called from within our own receive thread (remote disconnect path) - a thread
        // can't join itself, so let it run to completion on its own instead
        if( _thServerReceive.get_id() == std::this_thread::get_id() )
            _thServerReceive.detach();
        else
            _thServerReceive.join();
    }

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

            Stop();
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
    _thListen = std::thread( [this] { serverListenThread(); } );
}

Ext::Network::cNetworkServerReceiveHandler::~cNetworkServerReceiveHandler()
{
    _isStop = true;
    _eventQueue.Stop();

    if( _thListen.joinable() == true )
        _thListen.join();
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
