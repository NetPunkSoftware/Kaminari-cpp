#pragma once

#include <inttypes.h>


namespace kaminari
{
    class peer_based_phase_sync
    {
    public:
        peer_based_phase_sync();

        template <typename C>
        void on_receive_packet(C* client, uint16_t last_server_id);

        inline int16_t superpackets_id_diff() const;

    private:
        int16_t _superpackets_id_diff;
    };

    template <typename C>
    void peer_based_phase_sync::on_receive_packet(C* client, uint16_t last_server_id)
    {
        _superpackets_id_diff = (int16_t)((int)(client->super_packet()->id()) - (int)(last_server_id));
        client->protocol()->override_expected_block_id(last_server_id);
    }

    inline int16_t peer_based_phase_sync::superpackets_id_diff() const
    {
        return _superpackets_id_diff;
    }
}
