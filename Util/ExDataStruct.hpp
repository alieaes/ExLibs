#pragma once

#ifndef __HDR_EXT_DATA_STRUCT__
#define __HDR_EXT_DATA_STRUCT__

#include "Base/ExtConfig.hpp"

#include <queue>
#include <condition_variable>

namespace Ext
{
    namespace Queue
    {
        template< typename T >
        class eventQueue
        {
        public:
            eventQueue() {};
            ~eventQueue()
            {
                Stop();
            };

            T DeQueue()
            {
                std::unique_lock< std::mutex > lck( _lck );

                while( _queue.empty() == true )
                    _cvQueue.wait( lck );

                if( _isStop == true )
                    return T();

                T ret = _queue.front();
                _queue.pop();

                return ret;
            }

            void EnQueue( const T& item )
            {
                std::unique_lock< std::mutex > lck( _lck );

                _queue.push( item );
                _cvQueue.notify_one();
            }

            void Stop()
            {
                _isStop = true;
                _cvQueue.notify_one();
            }

        private:
            std::condition_variable                  _cvQueue;
            std::queue< T >                          _queue;
            std::mutex                               _lck;

            bool                                     _isStop = false;

        };
    }
}

#endif
