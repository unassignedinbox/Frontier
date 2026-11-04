/*==========================================================================================================================================
                                                       TRANSPORTRETRYVALIDATION.CPP
==========================================================================================================================================*/
// 🧩 Failed sends are retried in order, while incoming messages are still drained.

#include "Frontier/Extensions/Networking/NetworkingExtension.h"

#include <cstdio>
#include <utility>
#include <vector>

using namespace Frontier;

namespace
{
class RetryTransport final : public NetworkTransport
{
  public:
    bool Accept = false;
    std::vector<NetworkMessage> Sent{};
    std::vector<NetworkMessage> Incoming{};
    bool Send(const NetworkMessage& Message) override
    {
        if (!Accept) return false;
        Sent.push_back(Message);
        return true;
    }
    std::vector<NetworkMessage> Receive() override { return std::exchange(Incoming, {}); }
    const char* QueryName() const noexcept override { return "Retry test"; }
};

int Checks = 0, Failures = 0;
void Expect(bool Result, const char* Name)
{
    ++Checks;
    if (!Result) ++Failures;
    std::printf("%s %s\n", Result ? "PASS" : "FAIL", Name);
}
} // namespace

int main()
{
    NetworkingSpace Space;
    (void)Space.ConfigureLocalPeer("host", NetworkRole::Host);
    RetryTransport Transport;
    NetworkingExtension Bridge(Space, Transport);
    Space.Post(NetworkTopic::RecordConfiguration, "first");
    Space.Post(NetworkTopic::RecordRemoval, "second");
    Transport.Incoming.push_back({2, 0, NetworkTopic::Greeting, 9, "inbound"});
    Bridge.Exchange();
    Expect(Transport.Sent.empty(), "rejected send is not reported as accepted");
    const auto Incoming = Space.TakeIncoming();
    Expect(Incoming.size() == 1 && Incoming[0].Payload == "inbound", "receive still runs while send is blocked");
    Space.Post(NetworkTopic::RecordConfiguration, "third");
    Bridge.Exchange();
    Expect(Transport.Sent.empty(), "retries remain queued on repeated rejection");
    Transport.Accept = true;
    Bridge.Exchange();
    Expect(Transport.Sent.size() == 3, "all queued messages are accepted on retry");
    Expect(Transport.Sent.size() == 3 && Transport.Sent[0].Payload == "first" && Transport.Sent[1].Payload == "second" &&
           Transport.Sent[2].Payload == "third", "message order is preserved");
    Expect(Transport.Sent.size() == 3 && Transport.Sent[0].Sequence == 1 && Transport.Sent[1].Sequence == 2 &&
           Transport.Sent[2].Sequence == 3, "sequence numbers remain unchanged");
    Bridge.Exchange();
    Expect(Transport.Sent.size() == 3, "accepted messages are not resent");
    Expect(Space.TakeOutgoing().empty(), "space outgoing queue was drained into retry queue");
    std::printf("%d / %d checks passed\n", Checks - Failures, Checks);
    return Failures == 0 ? 0 : 1;
}
