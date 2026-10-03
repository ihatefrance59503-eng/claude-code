// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <chrono>

namespace fortnite
{
    template <typename T>
    class double_buffer_cache
    {
    public:
        double_buffer_cache( )
            : front_buffer( std::make_shared<T>( ) )
            , back_buffer( std::make_shared<T>( ) )
        {
            front_atomic.store( front_buffer , std::memory_order_release );
        }

        ~double_buffer_cache( )
        {
            stop( );
        }

        void start( std::function<void( T& )> update_fn , int sleep_us = 2500 )
        {
            update_function = update_fn;
            sleep_time_us = sleep_us;

            if ( running.exchange( true ) )
                return;

            worker = std::thread( &double_buffer_cache::run , this );
        }

        void stop( )
        {
            if ( !running.exchange( false ) )
                return;

            if ( worker.joinable( ) )
                worker.join( );
        }

        std::shared_ptr<const T> get( ) const
        {
            return front_atomic.load( std::memory_order_acquire );
        }

    private:
        void run( )
        {
            while ( running )
            {
                {
                    std::lock_guard<std::mutex> lock( mtx );

                    if ( update_function )
                        update_function( *back_buffer );

                    std::swap( front_buffer , back_buffer );
                    front_atomic.store( front_buffer , std::memory_order_release );
                }

                std::this_thread::sleep_for( std::chrono::microseconds( sleep_time_us ) );
            }
        }

    private:
        std::shared_ptr<T> front_buffer;
        std::shared_ptr<T> back_buffer;

        std::mutex mtx;
        std::thread worker;
        std::atomic<bool> running { false };
        std::atomic<std::shared_ptr<const T>> front_atomic;

        std::function<void( T& )> update_function;
        int sleep_time_us = 2500;
    };
}