//====================================================================================================================================
//                                                      LOBBYWORKSPACE.JS
//====================================================================================================================================
// 🧩 Lobby overlay: roster over BroadcastChannel, Ready to travel, and the car configurator wired to VehicleAppearance.Apply.

import { LobbyChannelName, LobbyMessage, MemberTimeoutMilliseconds, CreateRoster, JoinRoster, LeaveRoster, HeartbeatRoster,
         SetReady, ExpireRoster, AllReady, ListRoster } from "./RosterRegistry.js";
import { PaintChoices, DefaultChoice, ResolveAppearance } from "./AppearanceChoices.js";

// 📝 Next level after Ready. The prototype's next level is the proving ground renderer page at the renderer root.
const NextLevelPath = "/index.html";
const EnvironmentStorageKey = "Frontier.Environment.v1";
const BackupStorageKey = "Frontier.Lobby.EnvironmentBackup";
const HeartbeatMilliseconds = 1000;

const ById = (Id) => document.getElementById(Id);

const Self = {
    Id: Math.random().toString(36).slice(2, 10),
    Name: "",
    Ready: false,
    Joined: true,
    Choice: { ...DefaultChoice }
};
Self.Name = "Player " + Self.Id.slice(0, 4).toUpperCase();

let Roster = CreateRoster();
let Channel = null;
let Travelling = false;

function Post(Type, Extra = {}) {
    if (Channel)
        Channel.postMessage({ Type, Id: Self.Id, Name: Self.Name, At: Date.now(), ...Extra });
}

function Announce() {
    Roster = JoinRoster(Roster, { Id: Self.Id, Name: Self.Name }, Date.now());
    Roster = SetReady(Roster, Self.Id, Self.Ready);
    Post(LobbyMessage.Hello, { Ready: Self.Ready });
}

function OnMessage(Event) {
    const Message = Event.data ?? {};
    if (!Message.Id || Message.Id === Self.Id)
        return;
    const Now = Date.now();
    switch (Message.Type) {
        case LobbyMessage.Hello:
            Roster = JoinRoster(Roster, { Id: Message.Id, Name: Message.Name }, Now);
            Roster = SetReady(Roster, Message.Id, Boolean(Message.Ready));
            // 📝 Answer a newcomer so it learns about members that joined before it.
            Post(LobbyMessage.Heartbeat, { Ready: Self.Ready });
            break;
        case LobbyMessage.Heartbeat:
            Roster = HeartbeatRoster(JoinRoster(Roster, { Id: Message.Id, Name: Message.Name }, Now), Message.Id, Now);
            break;
        case LobbyMessage.Ready:
            Roster = SetReady(HeartbeatRoster(Roster, Message.Id, Now), Message.Id, Boolean(Message.Ready));
            break;
        case LobbyMessage.Goodbye:
            Roster = LeaveRoster(Roster, Message.Id);
            break;
        case LobbyMessage.Travel:
            Travel("Ready in every seat — a member started the next level.");
            return;
        default:
            return;
    }
    Render();
    TryTravel();
}

// 📝 Travel fires only when this tab is ready and every listed member is ready. A peer's travel message follows the same path.
function TryTravel() {
    if (!Self.Joined || !Self.Ready || Travelling)
        return;
    if (AllReady(Roster))
        Travel("All members are ready. Moving to the next level.", true);
}

function Travel(Notice, Broadcast = false) {
    if (Travelling)
        return;
    Travelling = true;
    if (Broadcast)
        Post(LobbyMessage.Travel);
    RestoreEnvironment();
    ById("lobby-status").textContent = Notice;
    setTimeout(() => { location.assign(NextLevelPath); }, Broadcast ? 400 : 0);
}

// 📝 Puts back the environment the level editor had saved before the lobby overwrote it, then removes the backup.
function RestoreEnvironment() {
    const Backup = localStorage.getItem(BackupStorageKey);
    if (Backup === null)
        return;
    if (Backup === "")
        localStorage.removeItem(EnvironmentStorageKey);
    else
        localStorage.setItem(EnvironmentStorageKey, Backup);
    localStorage.removeItem(BackupStorageKey);
}

function Render() {
    const Members = ListRoster(Roster);
    const List = ById("lobby-roster");
    List.replaceChildren(...Members.map((Member) => {
        const Row = document.createElement("li");
        Row.className = Member.Ready ? "ready" : "";
        const Name = document.createElement("span");
        Name.textContent = Member.Id === Self.Id ? Member.Name + " (you)" : Member.Name;
        const State = document.createElement("span");
        State.className = "state";
        State.textContent = Member.Ready ? "Ready" : "Not ready";
        Row.append(Name, State);
        return Row;
    }));
    ById("lobby-count").textContent = Members.length + (Members.length === 1 ? " player" : " players");
    const Waiting = Members.filter((Member) => !Member.Ready).length;
    if (!Self.Joined)
        ById("lobby-status").textContent = "You left the lobby. Join again to take part.";
    else if (Travelling)
        return;
    else if (Self.Ready && Waiting > 0)
        ById("lobby-status").textContent = "Ready. Waiting for " + Waiting + " more to press Ready.";
    else if (!Self.Ready)
        ById("lobby-status").textContent = "Pick a finish, then press Ready to start the next level.";
    ById("lobby-ready").textContent = Self.Ready ? "Not ready" : "Ready";
    ById("lobby-ready").disabled = !Self.Joined;
    ById("lobby-join").textContent = Self.Joined ? "Leave lobby" : "Join lobby";
}

function ReadChoice() {
    Self.Choice = {
        PaintIndex: Number(document.querySelector("input[name=paint]:checked")?.value ?? DefaultChoice.PaintIndex),
        Matte: ById("lobby-matte").checked,
        DarkWheels: ById("lobby-dark-wheels").checked,
        BlueRims: ById("lobby-blue-rims").checked
    };
}

// 📝 Applies the configurator to the single studio car. Each client renders its own copy, so the paint is local to this tab.
function ApplyConfiguration() {
    ReadChoice();
    const Host = window.IlluminationExhibit;
    const Appearance = Host?.Cabin?.Appearance;
    if (!Appearance)
        return;
    Appearance.Apply(ResolveAppearance(Self.Choice));
}

function BuildPaintChoices() {
    const Group = ById("lobby-paint");
    PaintChoices.forEach((Choice, Index) => {
        const Label = document.createElement("label");
        Label.className = "swatch";
        const Input = document.createElement("input");
        Input.type = "radio";
        Input.name = "paint";
        Input.value = String(Index);
        Input.checked = Index === DefaultChoice.PaintIndex;
        Input.addEventListener("change", ApplyConfiguration);
        const Chip = document.createElement("span");
        Chip.className = "chip";
        Chip.style.background = Choice.Hex;
        const Text = document.createElement("span");
        Text.textContent = Choice.Label;
        Label.append(Input, Chip, Text);
        Group.append(Label);
    });
}

function WireControls() {
    BuildPaintChoices();
    for (const Id of ["lobby-matte", "lobby-dark-wheels", "lobby-blue-rims"])
        ById(Id).addEventListener("change", ApplyConfiguration);
    ById("lobby-ready").addEventListener("click", () => {
        if (!Self.Joined || Travelling)
            return;
        Self.Ready = !Self.Ready;
        Roster = SetReady(Roster, Self.Id, Self.Ready);
        Post(LobbyMessage.Ready, { Ready: Self.Ready });
        Render();
        TryTravel();
    });
    ById("lobby-join").addEventListener("click", () => {
        if (Travelling)
            return;
        if (Self.Joined) {
            Post(LobbyMessage.Goodbye);
            Roster = LeaveRoster(Roster, Self.Id);
            Self.Joined = false;
            Self.Ready = false;
        } else {
            Self.Joined = true;
            Announce();
        }
        Render();
    });
}

function StartRoster() {
    Channel = new BroadcastChannel(LobbyChannelName);
    Channel.addEventListener("message", OnMessage);
    Announce();
    setInterval(() => {
        // 📝 This tab refreshes its own entry too, otherwise its own silence would expire it.
        if (Self.Joined) {
            Roster = HeartbeatRoster(Roster, Self.Id, Date.now());
            Post(LobbyMessage.Heartbeat, { Ready: Self.Ready });
        }
        Roster = ExpireRoster(Roster, Date.now(), MemberTimeoutMilliseconds).Roster;
        Render();
    }, HeartbeatMilliseconds);
    addEventListener("pagehide", () => { if (Self.Joined) Post(LobbyMessage.Goodbye); });
    Render();
}

function WaitForHost() {
    const Host = window.IlluminationExhibit;
    if (Host?.Ready) {
        ApplyConfiguration();
        return;
    }
    setTimeout(WaitForHost, 200);
}

WireControls();
StartRoster();
WaitForHost();
