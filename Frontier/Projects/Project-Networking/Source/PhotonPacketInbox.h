//============================================================================================================================================
//                                                        PHOTONPACKETINBOX.H
//============================================================================================================================================
// 📦 Bounded, validated packet handoff from the Photon callback to a later consumer, on the transport's tick thread.

#pragma once
#include "TransportCodec.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Networking
{
struct PhotonPacket
{
    int SenderSlot = 0;
    ReplicationKind Kind = ReplicationKind::Probe;
    std::vector<std::uint8_t> Payload;
};

class PhotonPacketInbox
{
  public:
    static constexpr std::size_t MaxPackets = 256;

    // False means malformed, full or allocation failed; no partial packet is added.
    bool Push(int SenderSlot, const std::uint8_t* Bytes, std::size_t Length) noexcept
    {
        ReplicationKind Kind;
        const std::uint8_t* Payload = nullptr;
        std::size_t Size = 0;
        if (SenderSlot <= 0 || Packets.size() >= MaxPackets || !PacketCodec::Decode(Bytes, Length, Kind, Payload, Size)) return false;
        try
        {
            PhotonPacket Packet;
            Packet.SenderSlot = SenderSlot;
            Packet.Kind = Kind;
            Packet.Payload.assign(Payload, Payload + Size);
            Packets.push_back(std::move(Packet));
            return true;
        }
        catch (...) { return false; }
    }

    std::vector<PhotonPacket> Take() noexcept { return std::exchange(Packets, {}); }
    void Clear() noexcept { Packets.clear(); }

  private:
    std::vector<PhotonPacket> Packets;
};
} // namespace Networking
