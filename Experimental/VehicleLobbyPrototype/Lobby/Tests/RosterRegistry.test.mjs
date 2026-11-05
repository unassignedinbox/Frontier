//====================================================================================================================================
//                                                  ROSTERREGISTRY.TEST.MJS
//====================================================================================================================================
// 🧩 Roster join, leave, heartbeat, expiry and ready rules. Run: node --test Lobby/Tests/

import test from "node:test";
import assert from "node:assert/strict";
import {
    CreateRoster, JoinRoster, LeaveRoster, HeartbeatRoster, SetReady, ExpireRoster, AllReady, ListRoster, MemberTimeoutMilliseconds
} from "../Source/RosterRegistry.js";
import { ResolveAppearance, PaintChoices, DefaultChoice } from "../Source/AppearanceChoices.js";

test("join adds a member once and keeps the ready state on a repeat join", () => {
    let Roster = JoinRoster(CreateRoster(), { Id: "a", Name: "Ada" }, 1000);
    Roster = SetReady(Roster, "a", true);
    Roster = JoinRoster(Roster, { Id: "a", Name: "Ada again" }, 2000);
    assert.equal(ListRoster(Roster).length, 1);
    assert.equal(Roster.Members.a.Ready, true);
    assert.equal(Roster.Members.a.Seen, 2000);
    assert.equal(Roster.Members.a.Name, "Ada again");
});

test("leave removes one member and ignores unknown ids", () => {
    let Roster = JoinRoster(CreateRoster(), { Id: "a" }, 0);
    Roster = JoinRoster(Roster, { Id: "b" }, 0);
    const Same = LeaveRoster(Roster, "missing");
    assert.equal(Same, Roster);
    Roster = LeaveRoster(Roster, "a");
    assert.deepEqual(Object.keys(Roster.Members), ["b"]);
});

test("heartbeat refreshes last-seen only for known members", () => {
    let Roster = JoinRoster(CreateRoster(), { Id: "a" }, 0);
    assert.equal(HeartbeatRoster(Roster, "ghost", 9000), Roster);
    Roster = HeartbeatRoster(Roster, "a", 3000);
    assert.equal(Roster.Members.a.Seen, 3000);
});

test("expiry removes members silent past the timeout and reports them", () => {
    let Roster = JoinRoster(CreateRoster(), { Id: "a" }, 0);
    Roster = JoinRoster(Roster, { Id: "b" }, 5000);
    const Result = ExpireRoster(Roster, MemberTimeoutMilliseconds + 1, MemberTimeoutMilliseconds);
    assert.deepEqual(Result.Removed, ["a"]);
    assert.deepEqual(Object.keys(Result.Roster.Members), ["b"]);
});

test("all ready requires a non-empty roster where every member is ready", () => {
    let Roster = CreateRoster();
    assert.equal(AllReady(Roster), false);
    Roster = JoinRoster(Roster, { Id: "a" }, 0);
    Roster = JoinRoster(Roster, { Id: "b" }, 0);
    Roster = SetReady(Roster, "a", true);
    assert.equal(AllReady(Roster), false);
    Roster = SetReady(Roster, "b", true);
    assert.equal(AllReady(Roster), true);
    Roster = LeaveRoster(Roster, "b");
    assert.equal(AllReady(Roster), true, "a departing member that was not ready must not block the rest");
});

test("appearance maps a valid choice to Apply arguments", () => {
    const Resolved = ResolveAppearance({ PaintIndex: 1, Matte: true, DarkWheels: true, BlueRims: false });
    assert.deepEqual(Resolved, { Colour: PaintChoices[1].Hex, Matte: true, DarkWheels: true, BlueRims: false });
});

test("appearance falls back to the default for out-of-range paint", () => {
    assert.equal(ResolveAppearance({ PaintIndex: 99 }).Colour, PaintChoices[DefaultChoice.PaintIndex].Hex);
    assert.equal(ResolveAppearance({ PaintIndex: -1 }).Colour, PaintChoices[DefaultChoice.PaintIndex].Hex);
});
