#pragma once

#ifndef __HDR_EXT_NETWORK__
#define __HDR_EXT_NETWORK__

#include <atomic>
#include <cstring>
#include <future>
#include <map>
#include <mutex>
#include <type_traits>

#include "Network/include/Socket/Socket.h"
#include "Network/include/Socket/TCPClient.h"
#include "Network/include/Socket/TCPServer.h"

#include "String/EXString.hpp"
#include "Util/ExDataStruct.hpp"

namespace Ext
{
    namespace Network
    {
        typedef std::shared_ptr< CTCPClient > spTCPClient;
        typedef std::shared_ptr< CTCPServer > spTCPServer;

        typedef unsigned int MSGID;

        // 패킷 페이로드는 원시 바이트로 주고받는다. 구조체를 보내려면 Send<T>/SendAndWait<T> 등의
        // 템플릿 오버로드를 쓰거나, 받은 쪽에서 ExtractPacketData<T>()로 꺼내 쓰면 된다.
        typedef std::vector<char> PacketData;
        typedef std::tuple< size_t, MSGID, PacketData > tuPacketMsg;
        typedef std::function< void( ASocket::Socket, tuPacketMsg )> fnHandler;

		// 폴링 간격
        constexpr size_t NETWORK_ACCEPT_POLL_MSEC = 200;

        tuPacketMsg convertReceiveMsg( std::vector<char> vecChar );
        std::vector<char> convertSendMsg( MSGID msgId, const PacketData& vecData );

        // 아무 구조체나 기본으로 써도 되게끔 제공하는 범용 패킷. 필요하면 각자 구조체를 따로 만들어 써도 된다.
        #pragma pack( push, 1 )
        struct NETWORK_DEFAULT_PACKET
        {
            unsigned int   uType                 = 0;      // 응용 계층에서 패킷 종류를 구분하는 용도
            unsigned int   uDataSize              = 0;      // szData 안에서 실제로 유효한 바이트 수
            char           szData[ 4096 ]         = { 0 };
        };
        #pragma pack( pop )

        // 수신한 패킷( tuPacketMsg )에서 임의의 trivially-copyable 타입을 꺼낸다.
        template< typename T >
        T ExtractPacketData( const tuPacketMsg& tuMsg )
        {
            static_assert( std::is_trivially_copyable<T>::value, "ExtractPacketData<T> requires a trivially copyable type" );

            T data{};
            const PacketData& vecRaw = std::get<2>( tuMsg );

            memcpy( &data, vecRaw.data(), ( std::min )( sizeof( T ), vecRaw.size() ) );

            return data;
        }

        enum eNetworkType
        {
            NETWORK_NONE   = 0,
            NETWORK_CLIENT = 1,
            NETWORK_SERVER = 2
        };

        struct NETWORK_INFO
        {
            XString                                  sIP;
            XString                                  sName;

            DWORD                                    dwPort;

            eNetworkType                             eType;

            spTCPClient                              spClient;
            spTCPServer                              spServer;

            bool isValid()
            {
                return sIP.IsEmpty() == false && dwPort != 0 && sName.IsEmpty() == false && eType != NETWORK_NONE;
            }
        };

        class cNetworkServerReceive;
        typedef std::shared_ptr< cNetworkServerReceive > spNetworkServerReceive;

        class cNetworkServerReceiveHandler;
        typedef std::shared_ptr< cNetworkServerReceiveHandler > spNetworkServerReceiveHandler;

        class cNetwork;

        struct NETWORK_CLIENT_INFO
        {
            XString                                  sUUID;
            ASocket::Socket                          connectionClient;
            spNetworkServerReceive                   spServerReceive;

            cNetwork*                                pNetwork;

            fnHandler                                fnServerHandler;
        };

        class cNetwork
        {
        public:
            cNetwork();
            cNetwork( NETWORK_INFO info );
            ~cNetwork();

        public:
            void                                     SetNetworkInfo( const NETWORK_INFO& info ) { _info = info; }
            void                                     SetGlobalServerHandler( fnHandler fn, bool isNeedUpdate = false );
            void                                     SetGlobalClientHandler( fnHandler fn );

            bool                                     Connect();

            bool                                     DisConnect();
            bool                                     DisConnect( const XString& sUUID );
            bool                                     DisConnectAll();

			// 서버 -> 단일 클라이언트 / 모든 클라이언트 (응답 수신하지 않음)
            bool                                     SendToClient( const XString& sUUID, const PacketData& vecData );
            bool                                     SendToAll( const PacketData& vecData );

			// 클라이언트 -> 서버. 응답 수신하지 않음. 패킷에 사용된 MSGID 반환 (실패 시 0)
            MSGID                                    Send( const PacketData& vecData );
			// 클라이언트 -> 서버, 동일한 MSGID를 가진 응답 패킷이 도착하거나 타임아웃이 경과할 때까지 블록
            bool                                     SendAndWait( const PacketData& vecData, tuPacketMsg& outResponse, unsigned int msTimeout = 5000 );

            // 구조체를 그대로 보내고 싶을 때 쓰는 편의 오버로드. T는 trivially-copyable 해야 한다
            // (POD 구조체, NETWORK_DEFAULT_PACKET 등). 내부적으로 메모리를 그대로 복사해서 위 바이트 버전으로 보낸다.
            template< typename T >
            bool SendToClient( const XString& sUUID, const T& data )
            {
                static_assert( std::is_trivially_copyable<T>::value, "SendToClient<T> requires a trivially copyable type" );

                const char* p = reinterpret_cast<const char*>( &data );
                return SendToClient( sUUID, PacketData( p, p + sizeof( T ) ) );
            }

            template< typename T >
            bool SendToAll( const T& data )
            {
                static_assert( std::is_trivially_copyable<T>::value, "SendToAll<T> requires a trivially copyable type" );

                const char* p = reinterpret_cast<const char*>( &data );
                return SendToAll( PacketData( p, p + sizeof( T ) ) );
            }

            template< typename T >
            MSGID Send( const T& data )
            {
                static_assert( std::is_trivially_copyable<T>::value, "Send<T> requires a trivially copyable type" );

                const char* p = reinterpret_cast<const char*>( &data );
                return Send( PacketData( p, p + sizeof( T ) ) );
            }

            template< typename T >
            bool SendAndWait( const T& data, tuPacketMsg& outResponse, unsigned int msTimeout = 5000 )
            {
                static_assert( std::is_trivially_copyable<T>::value, "SendAndWait<T> requires a trivially copyable type" );

                const char* p = reinterpret_cast<const char*>( &data );
                return SendAndWait( PacketData( p, p + sizeof( T ) ), outResponse, msTimeout );
            }

        private:
            friend class                             cNetworkServerReceive;

            void                                     networkLogging( const std::string& sLog );

            void                                     serverListenThread();
            void                                     clientReceiveThread();

            bool                                     _isStop                = false;
            NETWORK_INFO                             _info;

            spTCPClient                              _spTCPClient;
            spTCPServer                              _spTCPServer;

            std::thread                              _thServerListen;
            std::thread                              _thClientReceive;

            std::mutex                               _lckClientMap;
            std::map< XString, NETWORK_CLIENT_INFO > _mapUUIDToClientInfo;
            fnHandler                                _fnServerHandler;
            fnHandler                                _fnClientHandler;

            std::atomic< MSGID >                     _msgIdSeq              = 1;

            std::mutex                               _lckPendingRequests;
            std::map< MSGID, std::promise< tuPacketMsg > > _mapPendingRequests;
        };

        class cNetworkServerReceive
        {
        public:
            cNetworkServerReceive();
            cNetworkServerReceive( NETWORK_CLIENT_INFO info );
            ~cNetworkServerReceive();

            void                                     SetClientInfo( NETWORK_CLIENT_INFO info );

            void                                     Start();
            void                                     Stop();

        private:
            void                                     receiveThread();

            bool                                     _isStop                = false;
            std::thread                              _thServerReceive;
            NETWORK_CLIENT_INFO                      _clientInfo;

            spNetworkServerReceiveHandler            _spHandler;
        };

        class cNetworkServerReceiveHandler
        {
        public:
            cNetworkServerReceiveHandler();
            ~cNetworkServerReceiveHandler();

            void                                     SetClientSocket( ASocket::Socket socket ) { _connection = socket; }
            void                                     SetHandler( fnHandler fn ) { _fnHandler = fn; }
            void                                     SetUUID( XString sUUID ) { _sUUID = sUUID; }

            void                                     InsertPacket( tuPacketMsg tuMsg );
        private:
            void                                     serverListenThread();

            bool                                     _isStop = false;

            ASocket::Socket                          _connection;
            fnHandler                                _fnHandler;
            XString                                  _sUUID;

            std::thread                              _thListen;

            Queue::eventQueue< tuPacketMsg >         _eventQueue;
        };
    }
}

#endif