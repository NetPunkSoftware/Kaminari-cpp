#pragma once

#include <kaminari/super_packet.hpp>
#include <kaminari/protocol/protocol.hpp>
#include <kaminari/client/basic_client.hpp>

#include <type_traits>
#include <concepts>


namespace kaminari
{
    template <typename T>
    concept has_receive_callback = requires(T t)
    {
        { t.on_receive_packet((void*)nullptr, std::declval<uint16_t>()) };
    };

    template <typename T>
    concept stateful_callback = std::is_constructible_v<T> && std::is_move_constructible_v<T> &&
        has_receive_callback<T>;

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    class client : public basic_client, public StatefulCallbacks...
    {
    public:
        template <typename... Args>
        client(uint8_t resend_threshold, Args&&... allocator_args);

        inline void reset() noexcept;
        inline void clean() noexcept;

        template <typename TimeBase>
        inline void received_packet(uint16_t tick_id, const boost::intrusive_ptr<data_wrapper>& data);

        inline kaminari::super_packet<Queues>* super_packet();
        inline kaminari::protocol* protocol();

        template <typename T>
        static constexpr bool has_stateful_callback();

    protected:
        Marshal _marshal;
        kaminari::super_packet<Queues> _super_packet;
        kaminari::protocol _protocol;
    };


    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    template <typename... Args>
    client<Marshal, Queues, StatefulCallbacks...>::client(uint8_t resend_threshold, Args&&... allocator_args) :
        basic_client(),
        StatefulCallbacks()...,
        _marshal(),
        _super_packet(resend_threshold, std::forward<Args>(allocator_args)...),
        _protocol()
    {}

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    inline void client<Marshal, Queues, StatefulCallbacks...>::reset() noexcept
    {
        basic_client::reset();
        _marshal.reset();
        _super_packet.reset();
        _protocol.reset();
    }

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    inline void client<Marshal, Queues, StatefulCallbacks...>::clean() noexcept
    {
        _super_packet.clean();
    }

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    template <typename TimeBase>
    inline void client<Marshal, Queues, StatefulCallbacks...>::received_packet(uint16_t tick_id, const boost::intrusive_ptr<data_wrapper>& data)
    {
        super_packet_reader reader(data);

        if (reader.has_flag(super_packet_flags::handshake))
        {
            _pending_super_packets.clear();
        }
        else if (_protocol.is_out_of_order(reader.tick_id()))
        {
            // Make sure to ignore old packets
            return;
        }

        // Handle all acks now
        _protocol.template handle_acks<TimeBase>(tick_id, reader, this, _marshal, super_packet());

        // TODO(gpascualg): Ideally, we want to start searching from rbegin(), but then we can't insert
        auto it = _pending_super_packets.begin();
        while (it != _pending_super_packets.end())
        {
            if (cx::overflow::ge(it->id(), reader.id()))
            {
                break;
            }

            ++it;
        }

        _pending_super_packets.insert(it, reader);

        // Call all callbacks, if any
        (..., StatefulCallbacks::on_receive_packet(this, _pending_super_packets.back().id()));
    }

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    inline kaminari::super_packet<Queues>* client<Marshal, Queues, StatefulCallbacks...>::super_packet()
    {
        return &_super_packet;
    }

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    inline kaminari::protocol* client<Marshal, Queues, StatefulCallbacks...>::protocol()
    {
        return &_protocol;
    }

    template <typename Marshal, typename Queues, stateful_callback... StatefulCallbacks>
    template <typename T>
    constexpr bool client<Marshal, Queues, StatefulCallbacks...>::has_stateful_callback()
    {
        return (std::same_as<T, StatefulCallbacks> || ...);
    }
}
