#pragma once

#include <kaminari/cx/overflow.hpp>

#include <array>
#include <chrono>
#include <inttypes.h>
#include <optional>
#include <set>
#include <unordered_map>


namespace kaminari
{
    namespace buffers
    {
        class packet_reader;
    }

    class basic_client;

    // TODO(gpascualg): 200 is the default resync diff, magic number
    constexpr inline uint16_t resolution_table_size = 200 * 4;
    constexpr inline uint16_t resolution_table_diff = resolution_table_size - 1;

    class basic_protocol
    {
    public:
        static inline uint16_t superpacket_interval = 50;

    public:
        basic_protocol() noexcept;

        bool update() noexcept;
        void reset() noexcept;
        bool resolve(basic_client* client, buffers::packet_reader* packet, uint16_t block_id) noexcept;
        void reset_resolution_table(uint16_t block_id) noexcept;

        inline void scheduled_ping() noexcept;

        inline uint16_t last_tick_id_read() const noexcept;

        inline void set_timestamp(uint64_t timestamp, uint16_t block_id) noexcept;

        template <typename TimeBase, uint64_t interval>
        inline uint64_t block_timestamp(uint16_t block_id) noexcept;

        inline uint16_t max_blocks_until_resync() const noexcept;
        inline void max_blocks_until_resync(uint16_t value) noexcept;

        inline uint16_t max_blocks_until_disconnection() const noexcept;
        inline void max_blocks_until_disconnection(uint16_t value) noexcept;

        inline bool needs_ping() const noexcept;
        inline uint16_t ping_interval() const noexcept;
        inline void ping_interval(uint16_t value) noexcept;
        inline uint64_t timestamp_diff(uint64_t timestamp) noexcept;

        std::optional<std::chrono::steady_clock::time_point> super_packet_timestamp(uint16_t block_id) noexcept;

        inline void set_buffer_size(uint8_t buffer_size);

    protected:
        uint8_t _buffer_size;
        uint16_t _since_last_ping;
        uint16_t _since_last_recv;
        uint16_t _last_tick_id_read; 
        uint64_t _timestamp;
        uint16_t _timestamp_block_id;

        // Conflict resolution
        std::array<uint64_t, resolution_table_size> _resolution_table;
        uint16_t _oldest_resolution_block_id;
        uint16_t _oldest_resolution_position;

        // Configuration
        uint16_t _max_blocks_until_resync;
        uint16_t _max_blocks_until_disconnection;
        uint16_t _ping_interval;

        // Lag estimation
        std::array<std::chrono::steady_clock::time_point, resolution_table_size> _timestamps;
        uint16_t _timestamp_head_position;
        uint16_t _timestamp_head_id;
        uint16_t _last_confirmed_timestamp_id;
   };


    inline void basic_protocol::scheduled_ping() noexcept
    {
        _since_last_ping = 0;
    }

    inline uint16_t basic_protocol::last_tick_id_read() const noexcept
    {
        return _last_tick_id_read;
    }

    inline void basic_protocol::set_timestamp(uint64_t timestamp, uint16_t block_id) noexcept
    {
        _timestamp = timestamp;
        _timestamp_block_id = block_id;
    }

    template <typename TimeBase, uint64_t interval>
    inline uint64_t basic_protocol::block_timestamp(uint16_t block_id) noexcept
    {
        const uint64_t converted_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(TimeBase(interval)).count();

        if (block_id >= _timestamp_block_id)
        {
            return _timestamp + (block_id - _timestamp_block_id) * converted_interval;
        }

        return _timestamp - (_timestamp_block_id - block_id) * converted_interval;
    }

    inline uint16_t basic_protocol::max_blocks_until_resync() const noexcept
    {
        return _max_blocks_until_resync;
    }

    inline void basic_protocol::max_blocks_until_resync(uint16_t value) noexcept
    {
        _max_blocks_until_resync = value;
    }

    inline uint16_t basic_protocol::max_blocks_until_disconnection() const noexcept
    {
        return _max_blocks_until_disconnection;
    }

    inline void basic_protocol::max_blocks_until_disconnection(uint16_t value) noexcept
    {
        _max_blocks_until_disconnection = value;
    }

    inline bool basic_protocol::needs_ping() const noexcept
    {
        return _since_last_ping >= _ping_interval;
    }

    inline uint16_t basic_protocol::ping_interval() const noexcept
    {
        return _ping_interval;
    }

    inline void basic_protocol::ping_interval(uint16_t value) noexcept
    {
        _ping_interval = value;
    }
    
    inline uint64_t basic_protocol::timestamp_diff(uint64_t timestamp) noexcept
    {
        return _timestamp - timestamp;
    }

    inline void basic_protocol::set_buffer_size(uint8_t buffer_size)
    {
        _buffer_size = buffer_size;
    }
}
