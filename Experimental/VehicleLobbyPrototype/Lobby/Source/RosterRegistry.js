//====================================================================================================================================
//                                                       ROSTERREGISTRY.JS
//====================================================================================================================================
// 🧩 Lobby roster as plain data: who is in the lobby, whether they are ready, and when they were last heard from.

// 📝 Shared channel name for BroadcastChannel. Every lobby tab and the peer page post the same message shapes.
export const LobbyChannelName = "frontier-lobby-v1";

// 📝 A member that has not sent a heartbeat for this long [ms] is removed, so a closed tab leaves the roster on its own.
export const MemberTimeoutMilliseconds = 4000;

export const LobbyMessage = Object.freeze({
    Hello: "hello",
    Heartbeat: "heartbeat",
    Ready: "ready",
    Goodbye: "goodbye",
    Travel: "travel"
});

export function CreateRoster() {
    return { Members: {} };
}

// 📝 Joining twice is idempotent: the second join refreshes the name and the last-seen time but keeps the ready state.
export function JoinRoster(Roster, Member, Now) {
    const Existing = Roster.Members[Member.Id];
    const Next = {
        Id: Member.Id,
        Name: String(Member.Name ?? Member.Id).slice(0, 24),
        Ready: Existing ? Existing.Ready : false,
        Seen: Now
    };
    return { Members: { ...Roster.Members, [Member.Id]: Next } };
}

export function LeaveRoster(Roster, Id) {
    if (!(Id in Roster.Members))
        return Roster;
    const Remaining = { ...Roster.Members };
    delete Remaining[Id];
    return { Members: Remaining };
}

export function HeartbeatRoster(Roster, Id, Now) {
    const Existing = Roster.Members[Id];
    if (!Existing)
        return Roster;
    return { Members: { ...Roster.Members, [Id]: { ...Existing, Seen: Now } } };
}

export function SetReady(Roster, Id, Ready) {
    const Existing = Roster.Members[Id];
    if (!Existing)
        return Roster;
    return { Members: { ...Roster.Members, [Id]: { ...Existing, Ready: Boolean(Ready) } } };
}

// 📝 Drops every member whose last heartbeat is older than the timeout. Returns the roster and the removed ids.
export function ExpireRoster(Roster, Now, TimeoutMilliseconds = MemberTimeoutMilliseconds) {
    const Kept = {};
    const Removed = [];
    for (const [Id, Member] of Object.entries(Roster.Members)) {
        if (Now - Member.Seen > TimeoutMilliseconds)
            Removed.push(Id);
        else
            Kept[Id] = Member;
    }
    return { Roster: { Members: Kept }, Removed };
}

// 📝 Travel starts only when the roster is not empty and every listed member is ready.
export function AllReady(Roster) {
    const Members = Object.values(Roster.Members);
    return Members.length > 0 && Members.every((Member) => Member.Ready);
}

export function ListRoster(Roster) {
    return Object.values(Roster.Members).sort((A, B) => (A.Seen - B.Seen) || A.Id.localeCompare(B.Id));
}
