//============================================================================================================================================
//                                                          GLASSHUDSEQUENCE.JS
//============================================================================================================================================
// 🧩 3D Windshield Glass Lobby Player HUD (top-left of the interior front windshield glass).
//    Renders a crisp collimated smoked-glass holographic HUD onto a 768×512 canvas uploaded into the lower atlas band
//    of CabinSequence's ClusterTexture — letting the driver immediately see joined Lobby players straight on the glass
//    without looking down at the center console touchscreen.

const LogicalW = 384;
const LogicalH = 256;
const Scale    = 2;
const FontFamily = '"Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
const MonoFamily = '"JetBrains Mono", "SF Mono", ui-monospace, monospace';

export const GlassHudPixels = Object.freeze([LogicalW * Scale, LogicalH * Scale]); // [768, 512]

export class GlassHudSequence
{
    constructor()
    {
        this.Canvas = null;
        this.Context = null;
        this.LastSignature = "";
        this.Visible = false;

        if (typeof document !== "undefined")
        {
            const Canvas = document.createElement("canvas");
            Canvas.width  = GlassHudPixels[0];
            Canvas.height = GlassHudPixels[1];
            this.Canvas = Canvas;
            this.Context = Canvas.getContext("2d");
        }
    }

    SignatureFor(Live = {})
    {
        const Roster = Live.Roster || Live.LobbyRoster;
        if (!Roster || !Array.isArray(Roster.Players) || Roster.Players.length === 0)
            return "hidden";
        const PlayersKey = Roster.Players.map((P) => `${P.NameTag || P.Name}:${P.Connected ? 1 : 0}:${P.Ready ? 1 : 0}:${P.PingMs || 0}`).join("|");
        return `on|${Live.VenueShort || Live.VenueTitle || Live.VenueKey || ""}|${Roster.StageRaised ? 1 : 0}|${PlayersKey}`;
    }

    // 📝 Repaints the windshield glass HUD only when Lobby player state changes; returns true if GPU re-upload is needed.
    Advance(Live = {})
    {
        const Sig = this.SignatureFor(Live);
        if (Sig === this.LastSignature)
            return false;
        this.LastSignature = Sig;

        const C = this.Context;
        if (!C)
            return false;

        C.save();
        C.setTransform(1, 0, 0, 1, 0, 0);
        C.clearRect(0, 0, GlassHudPixels[0], GlassHudPixels[1]);

        if (Sig === "hidden")
        {
            this.Visible = false;
            C.restore();
            return true;
        }

        this.Visible = true;
        C.scale(Scale, Scale);

        const Roster = Live.Roster || Live.LobbyRoster;
        const Players = Roster.Players || [];
        const Connected = Players.filter((P) => P.Connected);
        const ReadyCount = Connected.filter((P) => P.Ready).length;
        const VenueName = String(Live.VenueShort || Live.VenueTitle || Live.VenueKey || "STUDIO").toUpperCase();

        // ① Collimated dark smoked-glass combiner panel (high contrast against bright sky AND dark studios)
        const Pad = 6;
        const PW = LogicalW - Pad * 2;
        const PH = LogicalH - Pad * 2;
        const R = 14;

        const BgGrad = C.createLinearGradient(Pad, Pad, Pad, Pad + PH);
        BgGrad.addColorStop(0.0, "rgba(6, 11, 19, 0.86)");
        BgGrad.addColorStop(1.0, "rgba(4, 8, 15, 0.82)");
        this.RoundRect(C, Pad, Pad, PW, PH, R);
        C.fillStyle = BgGrad;
        C.fill();

        // Crisp holographic ice-cyan glass border sheen
        C.lineWidth = 1.5;
        C.strokeStyle = "rgba(170, 232, 255, 0.44)";
        C.stroke();

        // Top glass specular edge highlight
        const TopShine = C.createLinearGradient(Pad + 16, Pad, Pad + PW - 16, Pad);
        TopShine.addColorStop(0.0, "rgba(140, 228, 255, 0.0)");
        TopShine.addColorStop(0.5, "rgba(195, 245, 255, 0.65)");
        TopShine.addColorStop(1.0, "rgba(140, 228, 255, 0.0)");
        C.fillStyle = TopShine;
        C.fillRect(Pad + 18, Pad + 1, PW - 36, 1.6);

        // Automotive collimated HUD corner reticle ticks
        this.DrawCornerTicks(C, Pad + 1, Pad + 1, PW - 2, PH - 2, 13, "#7ceaff");

        // ② Header strip: live pulse dot + "LOBBY PLAYERS" + online counter + active venue tag
        const HX = Pad + 15;
        const HY = Pad + 25;
        // Glowing emerald pulse dot
        C.beginPath();
        C.arc(HX + 4, HY - 4, 4.5, 0, Math.PI * 2);
        C.fillStyle = "#3ef5a3";
        C.fill();
        C.beginPath();
        C.arc(HX + 4, HY - 4, 7.6, 0, Math.PI * 2);
        C.fillStyle = "rgba(62, 245, 163, 0.25)";
        C.fill();

        C.font = `700 13.5px ${FontFamily}`;
        C.fillStyle = "#ffffff";
        C.textAlign = "left";
        C.textBaseline = "alphabetic";
        C.fillText("LOBBY PLAYERS", HX + 16, HY);

        // Online counter pill
        const CountLabel = `${Connected.length}/${Players.length} JOINED`;
        C.font = `700 11px ${MonoFamily}`;
        const CountW = C.measureText(CountLabel).width + 14;
        const PillX = HX + 134;
        this.RoundRect(C, PillX, HY - 13.5, CountW, 18, 9);
        C.fillStyle = "rgba(62, 245, 163, 0.20)";
        C.fill();
        C.strokeStyle = "rgba(62, 245, 163, 0.56)";
        C.lineWidth = 1.0;
        C.stroke();
        C.fillStyle = "#8affca";
        C.fillText(CountLabel, PillX + 7, HY - 0.5);

        // Right-aligned active venue badge
        C.font = `700 10.5px ${MonoFamily}`;
        C.fillStyle = "rgba(195, 230, 255, 0.92)";
        C.textAlign = "right";
        C.fillText(VenueName.slice(0, 14), Pad + PW - 14, HY - 0.5);

        // Subtle divider hairline
        C.fillStyle = "rgba(180, 224, 255, 0.18)";
        C.fillRect(Pad + 14, Pad + 36, PW - 28, 1);

        // ③ Player rows — joined players glow crisply at top, offline slots stay quiet & legible
        const Ordered = [
            ...Players.filter((P) => P.Connected),
            ...Players.filter((P) => !P.Connected)
        ].slice(0, 5);

        const RowStartY = Pad + 43;
        const RowH = 34;
        const RowGap = 4;

        Ordered.forEach((Player, Idx) =>
        {
            const RY = RowStartY + Idx * (RowH + RowGap);
            const RX = Pad + 13;
            const RW = PW - 26;
            const Joined = Boolean(Player.Connected);
            const Ready = Joined && Boolean(Player.Ready);
            const IsHost = Boolean(Player.IsLocal || Player.IsHero);
            const Callsign = String(Player.NameTag || Player.Name || "DRIVER");

            this.RoundRect(C, RX, RY, RW, RowH, 8);
            if (IsHost)
            {
                C.fillStyle = "rgba(121, 230, 255, 0.14)";
                C.fill();
                C.strokeStyle = "rgba(121, 230, 255, 0.44)";
                C.lineWidth = 1.0;
                C.stroke();
            }
            else if (Joined)
            {
                C.fillStyle = "rgba(255, 255, 255, 0.07)";
                C.fill();
                C.strokeStyle = "rgba(255, 255, 255, 0.14)";
                C.lineWidth = 0.9;
                C.stroke();
            }
            else
            {
                C.fillStyle = "rgba(255, 255, 255, 0.03)";
                C.fill();
            }

            // Status LED dot
            const DotColor = !Joined ? "rgba(145, 162, 185, 0.45)"
                           : Ready   ? "#3ef5a3"
                           : "#ffd166";
            C.beginPath();
            C.arc(RX + 14, RY + RowH * 0.5, 4.0, 0, Math.PI * 2);
            C.fillStyle = DotColor;
            C.fill();
            if (Joined)
            {
                C.beginPath();
                C.arc(RX + 14, RY + RowH * 0.5, 7.2, 0, Math.PI * 2);
                C.fillStyle = Ready ? "rgba(62, 245, 163, 0.25)" : "rgba(255, 209, 102, 0.24)";
                C.fill();
            }

            // Callsign + YOU/HOST tag
            C.font = `${Joined ? 700 : 500} 14.5px ${FontFamily}`;
            C.fillStyle = Joined ? "#ffffff" : "rgba(182, 196, 214, 0.56)";
            C.textAlign = "left";
            const DisplayName = IsHost ? `${Callsign} (YOU)` : Callsign;
            C.fillText(DisplayName, RX + 28, RY + 22);

            // Ping telemetry (joined players only)
            if (Joined)
            {
                C.font = `600 11.5px ${MonoFamily}`;
                C.fillStyle = "rgba(195, 224, 248, 0.85)";
                C.textAlign = "right";
                C.fillText(`${Player.PingMs || 14} ms`, RX + RW - 84, RY + 21.5);
            }

            // Right status badge (READY / JOINED / OFFLINE)
            const BadgeText = !Joined ? "OFFLINE" : (Ready ? "READY" : "JOINED");
            const BW = 66, BH = 21;
            const BX = RX + RW - BW - 8;
            const BY = RY + (RowH - BH) * 0.5;
            this.RoundRect(C, BX, BY, BW, BH, 6);
            if (!Joined)
            {
                C.fillStyle = "rgba(130, 146, 166, 0.18)";
                C.fill();
                C.fillStyle = "rgba(184, 198, 216, 0.65)";
            }
            else if (Ready)
            {
                C.fillStyle = "rgba(62, 245, 163, 0.22)";
                C.fill();
                C.strokeStyle = "rgba(62, 245, 163, 0.55)";
                C.lineWidth = 0.9;
                C.stroke();
                C.fillStyle = "#8bffcb";
            }
            else
            {
                C.fillStyle = "rgba(255, 209, 102, 0.22)";
                C.fill();
                C.strokeStyle = "rgba(255, 209, 102, 0.55)";
                C.lineWidth = 0.9;
                C.stroke();
                C.fillStyle = "#ffe89e";
            }
            C.font = `700 10.5px ${MonoFamily}`;
            C.textAlign = "center";
            C.fillText(BadgeText, BX + BW * 0.5, BY + 14.5);
        });

        // ④ Footer telemetry bar
        const FootY = Pad + PH - 9;
        C.font = `700 10px ${MonoFamily}`;
        C.textAlign = "left";
        C.fillStyle = "rgba(165, 222, 252, 0.85)";
        C.fillText(`WINDSHIELD HUD · ${ReadyCount}/${Connected.length} READY`, Pad + 15, FootY);
        C.textAlign = "right";
        C.fillStyle = Roster.StageRaised ? "#80ecff" : "rgba(184, 204, 226, 0.78)";
        C.fillText(Roster.StageRaised ? "STAGE ELEVATED" : "STAGE FLUSH", Pad + PW - 15, FootY);

        C.restore();
        return true;
    }

    RoundRect(C, X, Y, W, H, R)
    {
        C.beginPath();
        C.moveTo(X + R, Y);
        C.lineTo(X + W - R, Y);
        C.quadraticCurveTo(X + W, Y, X + W, Y + R);
        C.lineTo(X + W, Y + H - R);
        C.quadraticCurveTo(X + W, Y + H, X + W - R, Y + H);
        C.lineTo(X + R, Y + H);
        C.quadraticCurveTo(X, Y + H, X, Y + H - R);
        C.lineTo(X, Y + R);
        C.quadraticCurveTo(X, Y, X + R, Y);
        C.closePath();
    }

    DrawCornerTicks(C, X, Y, W, H, Len, Colour)
    {
        C.save();
        C.strokeStyle = Colour;
        C.lineWidth = 1.8;
        C.lineCap = "round";
        const Corners = [
            [[X, Y + Len], [X, Y], [X + Len, Y]],
            [[X + W - Len, Y], [X + W, Y], [X + W, Y + Len]],
            [[X, Y + H - Len], [X, Y + H], [X + Len, Y + H]],
            [[X + W - Len, Y + H], [X + W, Y + H], [X + W, Y + H - Len]]
        ];
        for (const [A, B, D] of Corners)
        {
            C.beginPath();
            C.moveTo(A[0], A[1]);
            C.lineTo(B[0], B[1]);
            C.lineTo(D[0], D[1]);
            C.stroke();
        }
        C.restore();
    }
}
