/*==========================================================================================================================================
                                                     PHOTONPACKETINBOXVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 Photon receive handoff without the proprietary SDK or a cloud connection.

#include "Frontier/Projects/Project-Networking/Source/PhotonTransport.h"
#include <cstdio>
#include <vector>

using namespace Networking;
namespace
{
int Checks = 0, Failures = 0;
void Expect(bool Value, const char* Name) { ++Checks; if (!Value) ++Failures; std::printf("%s %s\n", Value ? "PASS" : "FAIL", Name); }
}
int main()
{
    PhotonPacketInbox Inbox;
    std::vector<std::uint8_t> Frame;
    const std::uint8_t Message[] = {1, 2, 3};
    Expect(PacketCodec::Encode(ReplicationKind::Snapshot, Message, sizeof(Message), Frame), "snapshot frame encoded");
    Expect(Inbox.Push(7, Frame.data(), Frame.size()), "valid frame accepted");
    Frame.back() = 9;
    Expect(Inbox.Push(8, Frame.data(), Frame.size()), "second sender accepted");
    auto Packets = Inbox.Take();
    Expect(Packets.size() == 2 && Packets[0].SenderSlot == 7 && Packets[1].SenderSlot == 8, "sender slots and order retained");
    Expect(Packets.size() == 2 && Packets[0].Kind == ReplicationKind::Snapshot && Packets[0].Payload == std::vector<std::uint8_t>({1,2,3}) &&
           Packets[1].Payload == std::vector<std::uint8_t>({1,2,9}), "payload copied independently from callback memory");
    Expect(Inbox.Take().empty(), "drain empties the inbox");
    Expect(!Inbox.Push(0, Frame.data(), Frame.size()) && !Inbox.Push(7, Frame.data(), Frame.size()-1), "invalid sender and truncated frame refused");
    bool Filled = true;
    for (std::size_t Index = 0; Index < PhotonPacketInbox::MaxPackets; ++Index) Filled &= Inbox.Push(7, Frame.data(), Frame.size());
    Expect(Filled && !Inbox.Push(7, Frame.data(), Frame.size()), "queue accepts its bounded capacity and rejects overflow");
    Packets = Inbox.Take();
    Expect(Packets.size() == PhotonPacketInbox::MaxPackets, "overflow does not displace accepted packets");
    (void)Inbox.Push(7, Frame.data(), Frame.size());
    Inbox.Clear();
    Expect(Inbox.Take().empty(), "clear discards packets from an old session");
    Expect(!PhotonLinkAvailable() && TakePhotonPackets().empty(), "unlinked Photon stub reports no packets");
    std::printf("%d / %d checks passed\n", Checks-Failures, Checks);
    return Failures ? 1 : 0;
}
