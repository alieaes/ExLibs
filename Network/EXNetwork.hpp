#pragma once

#ifndef __HDR_EXT_NETWORK__
#define __HDR_EXT_NETWORK__

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

        typedef std::tuple< size_t, MSGID, XString > tuPacketMsg;
        typedef std::function< void( ASocket::Socket, tuPacketMsg )> fnHandler;

        tuPacketMsg convertReceiveMsg( std::vector<char> vecChar );

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

            bool                                     Connect();

            bool                                     DisConnect();
            bool                                     DisConnect( const XString& sUUID );
            bool                                     DisConnectAll();

        private:
            friend class                             cNetworkServerReceive;

            void                                     networkLogging( const std::string& sLog );

            void                                     serverListenThread();

            bool                                     _isStop                = false;
            NETWORK_INFO                             _info;

            spTCPClient                              _spTCPClient;
            spTCPServer                              _spTCPServer;

            std::thread                              _thServerListen;

            std::map< XString, NETWORK_CLIENT_INFO > _mapUUIDToClientInfo;
            fnHandler                                _fnServerHandler;
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

            Queue::eventQueue< tuPacketMsg >         _eventQueue;
        };
    }
}

#endif