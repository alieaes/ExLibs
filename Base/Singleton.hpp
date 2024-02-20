#ifndef __HDR_EXT_SINGLETON__
#define __HDR_EXT_SINGLETON__

#include "Base/ExtConfig.hpp"

#include <mutex>

namespace Ext
{
    namespace Singletons
    {
        template < typename T >
        class Singleton
        {
        public:

            static T* GetInstance()
            {
                if( _instance == NULLPTR )
                    std::call_once( _flag, []() { _instance = new T; } );

                return _instance;
            }

            static void DestroyInstance()
            {
                if( _instance != NULLPTR )
                {
                    delete _instance;
                    _instance = NULLPTR;
                }
            }

        private:
            Singleton() {}
            ~Singleton() {}

            Singleton( const Singleton& ) = delete;
            Singleton( Singleton&& ) = delete;
            Singleton& operator=( const Singleton& ) = delete;
            Singleton& operator=( Singleton&& ) = delete;

            static T* _instance;
            static std::once_flag _flag;
        };

        template < typename T >
        T* Singleton<T>::_instance = NULLPTR;

        template < typename T >
        std::once_flag Singleton<T>::_flag;
    }
}

#endif
