//============================================================================================================================================
//                                                            SCREENSEQUENCE.JS
//============================================================================================================================================
// 🧩 Professional minimal dark-glass & matte-obsidian bento automotive HMI (1200 × 750 reference canvas):
//    • Home Console: full-bleed 3D isometric dark slate map, cobalt navigation vector, minimal status bar,
//      floating right circular icon rail, and 4 dark frosted-glass dock cards.
//    • Obsidian Bento Views: connected Lobby player presence (Connected / Offline / Ping / Ready), 5 live
//      zero-reload Lobby Venues, 5 driving Circuits, Climate & Stage Lift smart controls, live 3D Garage
//      paint/rim configurator, Telemetry, Tyres, and System Settings.

import { CourseMaterial } from "../MaterialSpecification.js";
import { PaintColours } from "../VehicleAppearance.js";
import { Applications, LauncherDesigns, ScreenWidth, ScreenHeight, ResolveApplication, ResolveDesign } from "./AppSpecification.js";
import { LevelCatalog } from "../LevelStructure.js";
import { LobbyVenues } from "../LobbyStructure.js";

const FontFamily = '"DM Sans", "Inter", system-ui, -apple-system, sans-serif';

export class ScreenSequence
{
    constructor(Canvas, Icons, Announce)
    {
        this.Canvas = Canvas;
        this.Preview = null;
        this.Context = Canvas.getContext("2d", { alpha: false });
        this.Icons = Icons || {};
        this.Announce = Announce;
        this.Design = "console";
        this.Application = null;
        this.Regions = [];
        this.Hovered = "";
        this.Pressed = "";
        this.Pointer = null;
        this.FocusIndex = 0;
        this.Dirty = true;
        this.Revision = 0;
        this.Values = {
            steering: 50, height: 35, stiffness: 65, spoiler: 14, pressure: 220,
            temperature: 22, fan: 2, volume: 55, brightness: 100, laps: 8, zoom: 1
        };
        this.VehicleTab = "Steering";
        this.Locked = false;
        this.SeatMode = "Comfort";
        this.RoomReady = true;
        this.Invited = "";
        this.MediaPlaying = false;
        this.Nickname = "NOVA-7";
        this.MessageDraft = "";
        this.Messages = [];
        this.TextTarget = "";
        this.FinishColour = "#33363a";
        this.Notice = "";
        this.RouteRequest = null;   // Driving level or 'lobby', executed LIVE IN-PLACE by IlluminationHost
        this.VenueRequest = null;   // Lobby venue key ('studio'|'monolith'|'saltflats'|'oculus'|'nocturne'), executed LIVE IN-PLACE
        this.StageLiftRequest = null;
        this.TogglePeerRequest = null;
        this.Clock = "";
        this.ActiveTextInput = null;
        this.Live = null;           // { Speed, Temperatures, Pressures, Tread, States, LevelKey, LevelTitle, VenueKey, VenueTitle, SelectedRoute, Roster }
        this.Scale = 1.6;
        this.OffsetAcross = 0;
        this.OffsetDown = 0;
        this.PanelWidth = ScreenWidth;
        this.PanelHeight = ScreenHeight;
        this.PresentAspect(ScreenWidth / ScreenHeight);
    }

    PresentAspect(Aspect)
    {
        const Reference = ScreenWidth / ScreenHeight;
        this.PanelWidth = Aspect >= Reference ? ScreenHeight * Aspect : ScreenWidth;
        this.PanelHeight = Aspect >= Reference ? ScreenHeight : ScreenWidth / Aspect;
        this.OffsetAcross = (this.PanelWidth - ScreenWidth) / 2;
        this.OffsetDown = (this.PanelHeight - ScreenHeight) / 2;
        this.Canvas.width = Math.round(this.PanelWidth * this.Scale);
        this.Canvas.height = Math.round(this.PanelHeight * this.Scale);
        this.Dirty = true;
    }

    QueryPanelPoint(FractionAcross, FractionDown)
    {
        return { Across: FractionAcross * this.PanelWidth - this.OffsetAcross, Down: FractionDown * this.PanelHeight - this.OffsetDown };
    }

    QueryPanelFraction(Across, Down)
    {
        return { Across: (Across + this.OffsetAcross) / this.PanelWidth, Down: (Down + this.OffsetDown) / this.PanelHeight };
    }

    PresentDesign(Identifier)
    {
        if (!ResolveDesign(Identifier)) return false;
        this.Design = Identifier;
        this.PresentHome();
        return true;
    }

    PresentHome()
    {
        this.Application = null;
        this.Notice = "";
        this.Pointer = null;
        this.Hovered = "";
        this.Pressed = "";
        this.TextTarget = "";
        this.ActiveTextInput?.blur();
        this.Dirty = true;
        this.Announce?.();
    }

    PresentApplication(Identifier)
    {
        if (Identifier !== "venues" && !ResolveApplication(Identifier)) return false;
        this.Application = Identifier;
        this.Notice = "";
        this.Hovered = "";
        this.Pressed = "";
        this.TextTarget = "";
        this.ActiveTextInput?.blur();
        this.Dirty = true;
        this.Announce?.();
        return true;
    }

    AdvanceClock()
    {
        const Reading = new Date().toLocaleTimeString("en-GB", { hour: "2-digit", minute: "2-digit" });
        if (Reading !== this.Clock) { this.Clock = Reading; this.Dirty = true; }
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                             DRAWING PRIMITIVES
    //--------------------------------------------------------------------------------------------------------------------

    RasterRectangle(Left, Top, Width, Height, Radius, Colour, Stroke = null, LineWidth = 1)
    {
        const Context = this.Context;
        Context.beginPath();
        Context.roundRect(Left, Top, Width, Height, Radius);
        if (Colour)
        {
            Context.fillStyle = Colour;
            Context.fill();
        }
        if (Stroke)
        {
            Context.strokeStyle = Stroke;
            Context.lineWidth = LineWidth;
            Context.stroke();
        }
    }

    RasterCircle(CX, CY, Radius, Colour, Stroke = null, LineWidth = 1)
    {
        const Context = this.Context;
        Context.beginPath();
        Context.arc(CX, CY, Radius, 0, Math.PI * 2);
        if (Colour) { Context.fillStyle = Colour; Context.fill(); }
        if (Stroke) { Context.strokeStyle = Stroke; Context.lineWidth = LineWidth; Context.stroke(); }
    }

    RasterText(Text, Left, Top, Size = 20, Colour = "#eef2f8", Align = "left", Weight = 400)
    {
        const Context = this.Context;
        Context.font = `${Weight} ${Size}px ${FontFamily}`;
        Context.textBaseline = "alphabetic";
        Context.textAlign = Align;
        Context.fillStyle = Colour;
        Context.fillText(String(Text), Left, Top);
    }

    RasterRule(Left, Top, Width, Colour = "rgba(255,255,255,0.08)")
    {
        this.Context.fillStyle = Colour;
        this.Context.fillRect(Left, Top, Width, 1);
    }

    AdmitRegion(Identifier, Left, Top, Width, Height, Action, Extra = {})
    {
        this.Regions.push({ Identifier, Left, Top, Width, Height, Action, ...Extra });
    }

    RasterButton(Identifier, Text, Left, Top, Width, Height, Action, Colour = "#171a1f", Ink = "#e6ebf2", Stroke = "rgba(255,255,255,0.10)")
    {
        const Hover = this.Hovered === Identifier;
        this.RasterRectangle(Left, Top, Width, Height, 12, Hover ? "#232831" : Colour, Hover ? "rgba(255,255,255,0.28)" : Stroke);
        this.RasterText(Text, Left + Width / 2, Top + Height / 2 + 5, 14, Ink, "center", 500);
        this.AdmitRegion(Identifier, Left, Top, Width, Height, Action);
    }

    // 📝 Minimalist monochrome hairline vector icons (zero childish pastel artwork).
    RasterGlyph(Name, CX, CY, Size = 18, Colour = "#dfe6f0")
    {
        const C = this.Context;
        const R = Size * 0.5;
        C.save();
        C.strokeStyle = Colour;
        C.fillStyle = Colour;
        C.lineWidth = 1.65;
        C.lineCap = "round";
        C.lineJoin = "round";
        C.beginPath();

        if (Name === "lock")
        {
            C.roundRect(CX - R * 0.65, CY - R * 0.1, R * 1.3, R * 0.95, 2.5);
            C.stroke();
            C.beginPath();
            C.arc(CX, CY - R * 0.15, R * 0.42, Math.PI, 0);
            C.stroke();
        }
        else if (Name === "thermo")
        {
            C.moveTo(CX, CY - R * 0.8); C.lineTo(CX, CY + R * 0.35);
            C.stroke();
            C.beginPath();
            C.arc(CX, CY + R * 0.55, R * 0.30, 0, Math.PI * 2);
            C.fill();
            C.beginPath();
            C.roundRect(CX - R * 0.24, CY - R * 0.95, R * 0.48, R * 1.35, R * 0.24);
            C.stroke();
        }
        else if (Name === "light")
        {
            C.arc(CX - R * 0.15, CY, R * 0.45, -Math.PI * 0.5, Math.PI * 0.5, true);
            C.closePath();
            C.stroke();
            for (const DY of [-0.35, 0, 0.35])
            {
                C.beginPath();
                C.moveTo(CX + R * 0.25, CY + R * DY);
                C.lineTo(CX + R * 0.78, CY + R * DY);
                C.stroke();
            }
        }
        else if (Name === "users")
        {
            C.arc(CX - R * 0.22, CY - R * 0.32, R * 0.30, 0, Math.PI * 2);
            C.stroke();
            C.beginPath();
            C.arc(CX - R * 0.22, CY + R * 0.72, R * 0.58, Math.PI * 1.15, Math.PI * 1.85);
            C.stroke();
            C.beginPath();
            C.arc(CX + R * 0.44, CY - R * 0.20, R * 0.22, 0, Math.PI * 2);
            C.stroke();
        }
        else if (Name === "seat" || Name === "car")
        {
            C.moveTo(CX - R * 0.75, CY + R * 0.25);
            C.lineTo(CX - R * 0.48, CY - R * 0.32);
            C.lineTo(CX + R * 0.42, CY - R * 0.32);
            C.lineTo(CX + R * 0.78, CY + R * 0.25);
            C.closePath();
            C.stroke();
            C.beginPath();
            C.arc(CX - R * 0.42, CY + R * 0.42, R * 0.18, 0, Math.PI * 2);
            C.arc(CX + R * 0.42, CY + R * 0.42, R * 0.18, 0, Math.PI * 2);
            C.stroke();
        }
        else if (Name === "map" || Name === "nav")
        {
            C.moveTo(CX, CY - R * 0.75);
            C.lineTo(CX + R * 0.58, CY + R * 0.65);
            C.lineTo(CX, CY + R * 0.32);
            C.lineTo(CX - R * 0.58, CY + R * 0.65);
            C.closePath();
            C.stroke();
        }
        else if (Name === "garage")
        {
            C.moveTo(CX - R * 0.72, CY - R * 0.12);
            C.lineTo(CX, CY - R * 0.72);
            C.lineTo(CX + R * 0.72, CY - R * 0.12);
            C.lineTo(CX + R * 0.72, CY + R * 0.68);
            C.lineTo(CX - R * 0.72, CY + R * 0.68);
            C.closePath();
            C.stroke();
        }
        else if (Name === "sliders" || Name === "settings")
        {
            for (const [Row, Knob] of [[-0.42, -0.25], [0.0, 0.32], [0.42, -0.05]])
            {
                C.beginPath();
                C.moveTo(CX - R * 0.75, CY + R * Row);
                C.lineTo(CX + R * 0.75, CY + R * Row);
                C.stroke();
                C.beginPath();
                C.arc(CX + R * Knob, CY + R * Row, R * 0.16, 0, Math.PI * 2);
                C.fill();
            }
        }
        else if (Name === "arrowUR")
        {
            C.moveTo(CX - R * 0.42, CY + R * 0.42);
            C.lineTo(CX + R * 0.42, CY - R * 0.42);
            C.moveTo(CX - R * 0.08, CY - R * 0.42);
            C.lineTo(CX + R * 0.42, CY - R * 0.42);
            C.lineTo(CX + R * 0.42, CY + R * 0.08);
            C.stroke();
        }
        else if (Name === "plus")
        {
            C.moveTo(CX - R * 0.55, CY); C.lineTo(CX + R * 0.55, CY);
            C.moveTo(CX, CY - R * 0.55); C.lineTo(CX, CY + R * 0.55);
            C.stroke();
        }
        else if (Name === "minus")
        {
            C.moveTo(CX - R * 0.55, CY); C.lineTo(CX + R * 0.55, CY);
            C.stroke();
        }
        else if (Name === "back")
        {
            C.moveTo(CX + R * 0.25, CY - R * 0.5);
            C.lineTo(CX - R * 0.32, CY);
            C.lineTo(CX + R * 0.25, CY + R * 0.5);
            C.stroke();
        }
        else if (Name === "stage")
        {
            C.moveTo(CX - R * 0.72, CY + R * 0.55); C.lineTo(CX + R * 0.72, CY + R * 0.55);
            C.moveTo(CX, CY + R * 0.22); C.lineTo(CX, CY - R * 0.62);
            C.moveTo(CX - R * 0.35, CY - R * 0.25); C.lineTo(CX, CY - R * 0.62); C.lineTo(CX + R * 0.35, CY - R * 0.25);
            C.stroke();
        }
        else if (Name === "shield")
        {
            C.moveTo(CX, CY - R * 0.78);
            C.lineTo(CX + R * 0.62, CY - R * 0.48);
            C.lineTo(CX + R * 0.48, CY + R * 0.28);
            C.lineTo(CX, CY + R * 0.78);
            C.lineTo(CX - R * 0.48, CY + R * 0.28);
            C.lineTo(CX - R * 0.62, CY - R * 0.48);
            C.closePath();
            C.stroke();
        }
        C.restore();
    }

    RasterToggleSwitch(Identifier, On, Left, Top, Action)
    {
        const W = 72, H = 38;
        this.RasterRectangle(Left, Top, W, H, 19, On ? "#202724" : "#181a1d", On ? "rgba(205,229,220,0.28)" : "rgba(255,255,255,0.08)");
        const ThumbX = On ? (Left + W - 19) : (Left + 19);
        this.RasterCircle(ThumbX, Top + 19, 14, On ? "#cde5dc" : "#565d68");
        this.AdmitRegion(Identifier, Left - 6, Top - 6, W + 12, H + 12, Action);
    }

    RasterMinimalStatusBar()
    {
        const C = this.Context;
        const TimeLabel = this.Clock || "12:08";
        this.RasterText(`${TimeLabel}   ${this.Values.temperature}°C`, 42, 36, 15, "#dbe3ef", "left", 500);

        // Center Frontier emblem
        this.RasterGlyph("shield", ScreenWidth * 0.5, 30, 19, "rgba(220,232,248,0.88)");

        // Right monochrome signal, bluetooth, wifi, battery
        const RX = ScreenWidth - 134;
        for (let I = 0; I < 4; ++I)
        {
            const H = 5 + I * 3;
            C.fillStyle = I < 3 ? "#dbe3ef" : "rgba(219,227,239,0.32)";
            C.fillRect(RX + I * 5, 35 - H, 3, H);
        }
        // Wifi arcs
        C.strokeStyle = "#dbe3ef"; C.lineWidth = 1.4;
        for (const Rad of [4, 7.5])
        {
            C.beginPath();
            C.arc(RX + 42, 34, Rad, Math.PI * 1.22, Math.PI * 1.78);
            C.stroke();
        }
        C.fillStyle = "#dbe3ef";
        C.beginPath(); C.arc(RX + 42, 34.5, 1.4, 0, Math.PI * 2); C.fill();

        // Battery outline + mint fill
        this.RasterRectangle(RX + 64, 24, 24, 12, 3, null, "rgba(219,227,239,0.75)", 1.3);
        C.fillStyle = "#9ae3c4";
        C.fillRect(RX + 66.5, 26.5, 16, 7);
        C.fillStyle = "rgba(219,227,239,0.75)";
        C.fillRect(RX + 89, 27.5, 2.2, 5);
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                           3D ISOMETRIC DARK ARCHITECTURAL MAP BACKDROP (Images #3 & #4)
    //--------------------------------------------------------------------------------------------------------------------

    RasterDark3DMapBackdrop(RouteX = 590)
    {
        const C = this.Context;
        const Bg = C.createLinearGradient(0, 0, ScreenWidth, ScreenHeight);
        Bg.addColorStop(0, "#080d16");
        Bg.addColorStop(0.55, "#0d1524");
        Bg.addColorStop(1, "#070b12");
        C.fillStyle = Bg;
        C.fillRect(-this.OffsetAcross, -this.OffsetDown, this.PanelWidth, this.PanelHeight);

        // Subtle atmospheric horizon sheen
        const Halo = C.createRadialGradient(RouteX, 160, 20, RouteX, 240, 560);
        Halo.addColorStop(0, "rgba(56, 128, 255, 0.14)");
        Halo.addColorStop(1, "rgba(10, 18, 32, 0.0)");
        C.fillStyle = Halo;
        C.fillRect(0, 0, ScreenWidth, ScreenHeight);

        // Extruded dark architectural 3D city/circuit blocks on left & right of central highway corridor
        const Blocks = [
            // Left quarter
            [62,  112, 235, 128, 16],
            [316,  92, 224, 145, 20],
            [88,  260, 205, 148, 14],
            [314, 256, 228, 165, 18],
            [110, 424, 235, 130, 12],
            [365, 434, 175, 120, 15],
            // Right quarter
            [646,  96, 230, 142, 18],
            [898, 118, 210, 138, 15],
            [644, 258, 248, 168, 22],
            [912, 274, 198, 152, 16],
            [646, 444, 215, 118, 14],
            [884, 444, 210, 118, 12]
        ];
        for (const [X, Y, W, H, Depth] of Blocks)
        {
            // Ground drop shadow
            C.fillStyle = "rgba(2, 5, 10, 0.58)";
            C.fillRect(X + Depth * 0.7, Y + Depth * 0.9, W, H);
            // Extruded south wall
            C.fillStyle = "#0b1220";
            C.fillRect(X, Y + H - 2, W, Depth * 0.6);
            // Extruded east/west wall
            C.fillStyle = "#0e1728";
            C.fillRect(X + W - 2, Y, Depth * 0.45, H + Depth * 0.55);
            // Architectural rooftop face
            const TopGrad = C.createLinearGradient(X, Y, X + W, Y + H);
            TopGrad.addColorStop(0, "#151f33");
            TopGrad.addColorStop(1, "#0f1727");
            C.fillStyle = TopGrad;
            C.fillRect(X, Y, W, H);
            // Subtle chamfer hairline
            C.strokeStyle = "rgba(130, 168, 230, 0.10)";
            C.lineWidth = 1;
            C.strokeRect(X + 0.5, Y + 0.5, W - 1, H - 1);
        }

        // Central highway corridor
        C.fillStyle = "rgba(20, 32, 54, 0.65)";
        C.fillRect(RouteX - 24, 0, 48, ScreenHeight);

        // Glowing cobalt-blue navigation route beam (matching Image #4!)
        const BeamGlow = C.createLinearGradient(RouteX - 26, 0, RouteX + 26, 0);
        BeamGlow.addColorStop(0, "rgba(42, 138, 255, 0)");
        BeamGlow.addColorStop(0.5, "rgba(42, 138, 255, 0.42)");
        BeamGlow.addColorStop(1, "rgba(42, 138, 255, 0)");
        C.fillStyle = BeamGlow;
        C.fillRect(RouteX - 26, 220, 52, 420);

        const BeamCore = C.createLinearGradient(0, 220, 0, 620);
        BeamCore.addColorStop(0, "#59b3ff");
        BeamCore.addColorStop(0.5, "#2484ff");
        BeamCore.addColorStop(1, "rgba(36, 132, 255, 0.25)");
        C.fillStyle = BeamCore;
        C.fillRect(RouteX - 4.5, 220, 9, 390);

        // Circular blue chevron navigation cursor puck
        this.RasterCircle(RouteX, 222, 23, "rgba(48, 146, 255, 0.22)");
        this.RasterCircle(RouteX, 222, 15, "#2b8eff", "rgba(185, 225, 255, 0.7)", 1.5);
        C.fillStyle = "#ffffff";
        C.beginPath();
        C.moveTo(RouteX, 214);
        C.lineTo(RouteX + 6, 227);
        C.lineTo(RouteX, 224);
        C.lineTo(RouteX - 6, 227);
        C.closePath();
        C.fill();
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                       HOME CONSOLE (Panoramic Dark-Glass Automotive HMI — Images #3 & #4)
    //--------------------------------------------------------------------------------------------------------------------

    RasterHomeConsole()
    {
        const Live = this.Live || {};
        const Roster = Live.Roster || this.FallbackRoster();
        const ActiveVenue = LobbyVenues.find((V) => V.Key === (Live.VenueKey || "studio")) || LobbyVenues[0];
        const SelectedRouteKey = Live.SelectedRoute || "inversion";
        const SelectedRoute = LevelCatalog.find((L) => L.Key === SelectedRouteKey) || LevelCatalog[0];
        const PresentTitle = Live.LevelKey === "lobby" ? ActiveVenue.FullTitle : (Live.LevelTitle || "Proving Grounds");
        const DestTitle = Live.LevelKey === "lobby" ? SelectedRoute.Title.replace(" · Race to The Wall", "").replace(" · Sugar Siege", "").replace(" · Conveyor", "") : "Showroom Lobby";
        const DestKey = Live.LevelKey === "lobby" ? SelectedRoute.Key : "lobby";

        this.RasterDark3DMapBackdrop(594);
        this.RasterMinimalStatusBar();

        // Top-left Origin overlay
        this.RasterText(PresentTitle, 44, 114, 24, "#edf3fc", "left", 500);
        this.RasterText("Present Location", 44, 138, 13, "rgba(195, 212, 238, 0.58)", "left", 400);

        // Live Connected Lobby Players compact badge right below origin (tap to inspect full roster!)
        const BadgeW = 248, BadgeH = 40;
        const BadgeHover = this.Hovered === "home-roster-pill";
        this.RasterRectangle(44, 156, BadgeW, BadgeH, 20, BadgeHover ? "rgba(28,42,66,0.9)" : "rgba(14,22,38,0.82)", "rgba(140,180,235,0.22)");
        this.RasterCircle(64, 176, 4.5, "#6ee7b7");
        this.RasterText(`${Roster.ActiveCount} / ${Roster.MaxCount} Players Connected`, 78, 181, 13, "#e4eefc", "left", 500);
        this.AdmitRegion("home-roster-pill", 44, 156, BadgeW, BadgeH, () => this.PresentApplication("drive-room"));

        // Bottom-left Destination overlay (tap to launch directly in-place!)
        const DestHover = this.Hovered === "home-launch-dest";
        this.RasterText(DestTitle, 44, 434, 24, DestHover ? "#7cc0ff" : "#edf3fc", "left", 500);
        this.RasterText(Live.LevelKey === "lobby" ? "Destination · Tap to launch live" : "Return to Showroom Lobby", 44, 458, 13, "rgba(195, 212, 238, 0.58)", "left", 400);
        this.AdmitRegion("home-launch-dest", 38, 404, 340, 66, () => { this.RouteRequest = DestKey; this.Dirty = true; });

        // Bottom-right Telemetry distance / speed
        const SpeedKmh = Math.round(Live.Speed || 0);
        const MetaRight = Live.LevelKey === "lobby" ? `Venue: ${ActiveVenue.Title} · 0 ms switch` : `Live speed: ${SpeedKmh} km/h`;
        this.RasterText(MetaRight, 1152, 464, 13, "rgba(195, 212, 238, 0.65)", "right", 400);

        // Right Floating Circular Frosted Icon Rail (5 buttons matching Image #3 & #4!)
        const Rail = [
            { Id: "rail:nav",      Glyph: "nav",     App: "navigation" },
            { Id: "rail:lobby",    Glyph: "users",   App: "drive-room" },
            { Id: "rail:venues",   Glyph: "light",   App: "venues" },
            { Id: "rail:garage",   Glyph: "garage",  App: "garage" },
            { Id: "rail:settings", Glyph: "sliders", App: "settings" }
        ];
        Rail.forEach((Item, Index) =>
        {
            const CX = 1126, CY = 128 + Index * 60;
            const Hover = this.Hovered === Item.Id;
            this.RasterCircle(CX, CY, 22, Hover ? "rgba(44, 68, 108, 0.92)" : "rgba(22, 34, 56, 0.80)",
                              Hover ? "rgba(145, 195, 255, 0.45)" : "rgba(135, 175, 235, 0.22)", 1.1);
            this.RasterGlyph(Item.Glyph, CX, CY, 16, "#e5eefc");
            this.AdmitRegion(Item.Id, CX - 24, CY - 24, 48, 48, () => this.PresentApplication(Item.App));
        });

        // Bottom 4-Column Dark Frosted-Glass Bento Dock Cards (matching Images #3 & #4!)
        const Cards = [
            {
                Id: "dock:lobby",
                Title: "Lobby\nPlayers",
                Glyph: "users",
                Footer: [`${Roster.ActiveCount} Online`, `${Roster.MaxCount - Roster.ActiveCount} Offline`, `${Roster.ReadyCount} Ready`],
                Action: () => this.PresentApplication("drive-room")
            },
            {
                Id: "dock:climate",
                Title: "Temperature\nSettings",
                Glyph: "thermo",
                Footer: [`Cabin ${this.Values.temperature}°C`, `Fan ${this.Values.fan}`, this.Locked ? "Locked" : "Unlocked"],
                Action: () => this.PresentApplication("climate")
            },
            {
                Id: "dock:venue",
                Title: "Lobby Venue\nSettings",
                Glyph: "light",
                Footer: [ActiveVenue.Title, "10 Venues", "Instant"],
                Action: () => this.PresentApplication("venues")
            },
            {
                Id: "dock:garage",
                Title: "Vehicle &\nFinish",
                Glyph: "car",
                Footer: [this.Appearance?.Matte ? "Matte" : "Gloss", Roster.StageRaised ? "Stage Up" : "Flush", "3D Live"],
                Action: () => this.PresentApplication("garage")
            }
        ];

        const CardW = 266, CardH = 186, Gap = 16, StartX = 44, CardY = 508;
        Cards.forEach((Card, Index) =>
        {
            const X = StartX + Index * (CardW + Gap);
            const Hover = this.Hovered === Card.Id;
            const Fill = Hover ? "rgba(22, 34, 56, 0.92)" : "rgba(13, 21, 36, 0.86)";
            const Border = Hover ? "rgba(128, 178, 245, 0.42)" : "rgba(130, 168, 224, 0.18)";
            this.RasterRectangle(X, CardY, CardW, CardH, 16, Fill, Border, 1.1);

            const Lines = Card.Title.split("\n");
            this.RasterText(Lines[0], X + 24, CardY + 52, 22, "#eef4fc", "left", 400);
            if (Lines[1])
                this.RasterText(Lines[1], X + 24, CardY + 79, 22, "#eef4fc", "left", 400);

            this.RasterGlyph(Card.Glyph, X + CardW - 34, CardY + 56, 18, "rgba(220, 232, 248, 0.85)");

            // Quiet metadata footer strip matching Image #3 & #4
            this.RasterRule(X + 24, CardY + CardH - 46, CardW - 48, "rgba(255, 255, 255, 0.08)");
            this.RasterText(Card.Footer[0], X + 24, CardY + CardH - 22, 12.5, "rgba(188, 206, 232, 0.62)", "left", 400);
            this.RasterText(Card.Footer[1], X + CardW * 0.5, CardY + CardH - 22, 12.5, "rgba(188, 206, 232, 0.56)", "center", 400);
            this.RasterText(Card.Footer[2], X + CardW - 24, CardY + CardH - 22, 12.5, "rgba(188, 206, 232, 0.62)", "right", 400);

            this.AdmitRegion(Card.Id, X, CardY, CardW, CardH, Card.Action);
        });
    }

    FallbackRoster()
    {
        return {
            ActiveCount: 4, ReadyCount: 3, MaxCount: 5, StageRaised: false,
            Players: [
                { Index: 0, NameTag: "NOVA-7",  Rank: "DIAMOND", Connected: true,  Ready: true,  PingMs: 8,  IsLocal: true,  Role: "Host · In Cockpit" },
                { Index: 1, NameTag: "KESTREL", Rank: "GOLD",    Connected: true,  Ready: true,  PingMs: 19, IsLocal: false, Role: "Connected · Lobby" },
                { Index: 2, NameTag: "MIRAGE",  Rank: "SILVER",  Connected: true,  Ready: false, PingMs: 31, IsLocal: false, Role: "Connected · Lobby" },
                { Index: 3, NameTag: "ORION",   Rank: "GOLD",    Connected: true,  Ready: true,  PingMs: 24, IsLocal: false, Role: "Connected · Lobby" },
                { Index: 4, NameTag: "VEGA-9",  Rank: "BRONZE",  Connected: false, Ready: false, PingMs: 0,  IsLocal: false, Role: "Offline" }
            ]
        };
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                     MATTE OBSIDIAN BENTO FRAME + SUPERSCRIPT TABS (Image #2)
    //--------------------------------------------------------------------------------------------------------------------

    RasterObsidianBackground()
    {
        const C = this.Context;
        C.fillStyle = "#090a0c";
        C.fillRect(-this.OffsetAcross, -this.OffsetDown, this.PanelWidth, this.PanelHeight);
    }

    RasterAppHeader(ActiveApp)
    {
        this.RasterMinimalStatusBar();

        // Back button to Home Console
        const BackHover = this.Hovered === "back-home";
        this.RasterRectangle(42, 60, 108, 38, 19, BackHover ? "#1f2228" : "#131518", "rgba(255,255,255,0.10)");
        this.RasterGlyph("back", 64, 79, 13, "#e5eaf2");
        this.RasterText("Console", 82, 84, 14, "#e5eaf2", "left", 500);
        this.AdmitRegion("back-home", 42, 60, 108, 38, () => this.PresentHome());

        // Minimal room/section tabs with superscript counts (exact aesthetic of Image #2: "Living Room⁴  Kitchen²  Patio²")
        const Live = this.Live || {};
        const Roster = Live.Roster || this.FallbackRoster();
        const Tabs = [
            { Id: "drive-room", Label: "Lobby",    Count: String(Roster.ActiveCount) },
            { Id: "venues",     Label: "Venues",   Count: String(LobbyVenues.length) },
            { Id: "navigation", Label: "Circuits", Count: String(LevelCatalog.length) },
            { Id: "garage",     Label: "Garage",   Count: "" },
            { Id: "climate",    Label: "Controls", Count: "4" },
            { Id: "settings",   Label: "System",   Count: "" }
        ];
        let TX = 186;
        for (const Tab of Tabs)
        {
            const Active = ActiveApp === Tab.Id;
            const Hover = this.Hovered === `tab:${Tab.Id}`;
            const Ink = Active ? "#ffffff" : (Hover ? "#cfd6e0" : "#6b7280");
            this.RasterText(Tab.Label, TX, 85, 18, Ink, "left", Active ? 500 : 400);
            const W = this.Context.measureText(Tab.Label).width;
            if (Tab.Count)
                this.RasterText(Tab.Count, TX + W + 4, 75, 11, Active ? "#cde5dc" : "#555c68", "left", 500);
            if (Active)
                this.RasterRectangle(TX, 96, W, 2, 1, "#ffffff");
            this.AdmitRegion(`tab:${Tab.Id}`, TX - 8, 58, W + 28, 42, () => this.PresentApplication(Tab.Id));
            TX += W + 52;
        }
    }

    RasterApplication()
    {
        this.RasterObsidianBackground();
        this.RasterAppHeader(this.Application);
        this.RasterApplicationContent(this.Application);
    }

    RasterApplicationContent(Identifier)
    {
        const Live = this.Live || {};
        const Roster = Live.Roster || this.FallbackRoster();

        // 1. LOBBY PLAYERS (Connected / Offline Roster — Image #2 Obsidian Bento!)
        if (Identifier === "drive-room" || Identifier === "contacts" || Identifier === "messages" || Identifier === "account")
        {
            const SelectedRouteKey = Live.SelectedRoute || "inversion";
            const SelectedRoute = LevelCatalog.find((L) => L.Key === SelectedRouteKey) || LevelCatalog[0];

            // Top row: Obsidian "Lobby Session" card + High-Contrast Pure-White "Drive Circuit" hero card (exact Image #2 contrast!)
            this.RasterRectangle(42, 122, 548, 152, 24, "#121316", "#202227", 1.1);
            this.RasterText("Lobby Session", 70, 178, 30, "#f2f4f8", "left", 400);
            this.RasterText(`${Roster.ActiveCount} of ${Roster.MaxCount} Players Connected · ${Roster.ReadyCount} Ready`, 70, 212, 15, "#8b929e", "left", 400);
            this.RasterText("Tap any player card below to inspect or toggle connection state.", 70, 246, 13, "#5d6470", "left", 400);
            this.RasterCircle(536, 196, 26, "#1b1d22", "#2b2e36");
            this.RasterGlyph("users", 536, 196, 18, "#e6ebf4");

            // Pure White active CTA card (matching "Office Light · On · Soft Mode" in Image #2!)
            const LaunchHover = this.Hovered === "bento-launch";
            this.RasterRectangle(610, 122, 548, 152, 24, LaunchHover ? "#f0f4f8" : "#ffffff");
            this.RasterText(SelectedRoute.Title.replace(" · Race to The Wall", "").replace(" · Sugar Siege", ""), 642, 178, 30, "#0b0c0e", "left", 500);
            this.RasterText("Live Switch · Zero Reload · Click to Drive", 642, 212, 15, "#474d57", "left", 400);
            this.RasterCircle(1102, 196, 26, "#0f1115");
            this.RasterGlyph("arrowUR", 1102, 196, 18, "#ffffff");
            this.AdmitRegion("bento-launch", 610, 122, 548, 152, () => { this.RouteRequest = SelectedRoute.Key; this.Dirty = true; });

            // Section header
            this.RasterText("Connected Players", 44, 314, 15, "#7e8694", "left", 500);

            // 5 Player Bento Tiles (3 top + 2 bottom, ultra-clean matte obsidian cards with mint Connected switch!)
            const Players = Roster.Players || this.FallbackRoster().Players;
            Players.forEach((Player, Index) =>
            {
                const Col = Index % 3;
                const Row = Math.floor(Index / 3);
                const W = 360, H = 178, Gap = 18;
                const X = 42 + Col * (W + Gap);
                const Y = 334 + Row * (H + 18);
                const CardId = `peer:${Player.Index}`;
                const Hover = this.Hovered === CardId;
                const Border = Player.Connected ? (Hover ? "rgba(205,229,220,0.45)" : "rgba(205,229,220,0.18)") : "#1e2025";
                this.RasterRectangle(X, Y, W, H, 22, Hover ? "#17191e" : "#121316", Border, 1.1);

                // Top-left avatar badge + live dot
                this.RasterCircle(X + 38, Y + 40, 22, "#1c1e23");
                this.RasterText(Player.NameTag.slice(0, 2), X + 38, Y + 45, 13, Player.Connected ? "#e8eff8" : "#666e7a", "center", 600);
                this.RasterCircle(X + 54, Y + 54, 5.5, Player.Connected ? "#6ee7b7" : "#444954", "#121316", 2);

                // Top-right rank pill
                this.RasterRectangle(X + W - 98, Y + 24, 76, 26, 13, "#1a1c21");
                this.RasterText(Player.Rank, X + W - 60, Y + 41, 10.5, Player.Connected ? "#b5c2d4" : "#5e6573", "center", 600);

                // Player name & live status
                this.RasterText(Player.IsLocal ? `${Player.NameTag} (You)` : Player.NameTag, X + 24, Y + 104, 22, Player.Connected ? "#f0f3f8" : "#787f8c", "left", 500);
                const Sub = Player.Connected
                    ? `Connected · ${Player.PingMs} ms · ${Player.Ready ? "Ready" : "Standby"}`
                    : "Offline · Not in Lobby";
                this.RasterText(Sub, X + 24, Y + 128, 13.5, Player.Connected ? "#99b3a9" : "#59606d", "left", 400);

                // Mint toggle switch (exact Image #2 toggle style!)
                this.RasterToggleSwitch(CardId, Player.Connected, X + W - 96, Y + H - 54, () =>
                {
                    this.TogglePeerRequest = Player.Index;
                    this.Dirty = true;
                });
            });
            return;
        }

        // 2. LOBBY VENUES (10 Pure Architectural Venues — 5 × 2 Matte-Obsidian & White Bento Grid!)
        if (Identifier === "venues")
        {
            const ActiveKey = Live.VenueKey || "studio";
            this.RasterText("Showroom Environments", 44, 152, 26, "#f0f3f8", "left", 400);
            this.RasterText("10 architectural venues · tap any tile to switch live in-place without reloading.", 44, 178, 14, "#7f8794", "left", 400);

            LobbyVenues.forEach((Venue, Index) =>
            {
                const Col = Index % 5;
                const Row = Math.floor(Index / 5);
                const W = 212, H = 222, GapX = 14, GapY = 16;
                const X = 42 + Col * (W + GapX);
                const Y = 204 + Row * (H + GapY);
                const Active = Venue.Key === ActiveKey && Live.LevelKey === "lobby";
                const Id = `venue:${Venue.Key}`;
                const Hover = this.Hovered === Id;

                this.RasterRectangle(X, Y, W, H, 22, Active ? "#ffffff" : (Hover ? "#181b21" : "#121316"), Active ? "#ffffff" : "#22252b", 1.1);
                this.RasterCircle(X + 34, Y + 38, 18, Active ? "#eef1f5" : "#1c1f25");
                this.RasterGlyph("light", X + 34, Y + 38, 14, Active ? "#0b0c0e" : "#d8e0ec");

                this.RasterCircle(X + W - 32, Y + 38, 18, Active ? "#0b0c0e" : "#1a1d22");
                this.RasterGlyph("arrowUR", X + W - 32, Y + 38, 13, Active ? "#ffffff" : "#9aa4b5");

                this.RasterText(Venue.Title, X + 20, Y + 118, 20, Active ? "#0b0c0e" : "#f0f3f8", "left", 500);
                // Split subtitle concisely onto 2 clean lines
                const Parts = String(Venue.Subtitle || "").split(" · ");
                this.RasterText(Parts[0] || "", X + 20, Y + 144, 11.5, Active ? "#4b525e" : "#808896", "left", 400);
                if (Parts[1])
                    this.RasterText(Parts[1].slice(0, 27), X + 20, Y + 162, 11, Active ? "#636b78" : "#646d7b", "left", 400);
                this.RasterText(Active ? "ACTIVE VENUE" : "TAP TO SWITCH", X + 20, Y + 198, 10.5, Active ? "#0b0c0e" : "#cde5dc", "left", 600);

                this.AdmitRegion(Id, X, Y, W, H, () => { this.VenueRequest = Venue.Key; this.Dirty = true; });
            });
            return;
        }

        // 3. NAVIGATION / DRIVING CIRCUITS (Dark 3D Map + Live Zero-Reload Route Cards!)
        if (Identifier === "navigation")
        {
            this.RasterDark3DMapBackdrop(340);
            this.RasterAppHeader("navigation");

            this.RasterText("Driving Circuits", 44, 152, 24, "#edf3fc", "left", 500);
            this.RasterText("Select any destination to stream live in-place without reloading.", 44, 176, 13.5, "rgba(195,212,238,0.62)", "left", 400);

            const Destinations = [
                { Key: "lobby", Title: "Showroom Lobby", Subtitle: "10 single-hero architectural venues · HMI" },
                ...LevelCatalog
            ];
            Destinations.forEach((Dest, Index) =>
            {
                const X = 610, Y = 118 + Index * 78, W = 548, H = 68;
                const Id = `route:${Dest.Key}`;
                const Active = Live.LevelKey === Dest.Key;
                const Hover = this.Hovered === Id;
                this.RasterRectangle(X, Y, W, H, 16, Active ? "#ffffff" : (Hover ? "rgba(26,38,60,0.92)" : "rgba(14,21,34,0.88)"),
                                     Active ? "#ffffff" : "rgba(135,175,235,0.20)", 1.1);
                this.RasterText(Dest.Title, X + 24, Y + 30, 17.5, Active ? "#0b0c0e" : "#edf3fc", "left", 500);
                this.RasterText(Dest.Subtitle, X + 24, Y + 50, 11.5, Active ? "#4d5562" : "rgba(186,204,230,0.60)", "left", 400);
                this.RasterCircle(X + W - 36, Y + 34, 17, Active ? "#0b0c0e" : "rgba(32,48,74,0.9)");
                this.RasterGlyph("arrowUR", X + W - 36, Y + 34, 13, "#ffffff");
                this.AdmitRegion(Id, X, Y, W, H, () => { this.RouteRequest = Dest.Key; this.Dirty = true; });
            });
            return;
        }

        // 4. CONTROLS & CLIMATE (Exact Image #2 Matte Obsidian Smart Bento Cards!)
        if (Identifier === "climate" || Identifier === "vehicle" || Identifier === "media" || Identifier === "tyres" || Identifier === "telemetry")
        {
            // Row 1: Stage Hydraulic Lift + Cabin Lock (matching "Smart TV" toggle & "Office Light" active white card!)
            const StageOn = Boolean(Roster.StageRaised);
            this.RasterRectangle(42, 128, 360, 268, 24, "#121316", "#202227", 1.1);
            this.RasterCircle(84, 174, 24, "#1c1e23");
            this.RasterGlyph("stage", 84, 174, 18, "#e6ecf5");
            this.RasterText("Hydraulic Stage", 66, 260, 24, "#f0f3f8", "left", 400);
            this.RasterText(StageOn ? "Elevated · +0.42 m Showcase" : "Flush · Seamless Floor", 66, 288, 14, "#858d9b", "left", 400);
            this.RasterToggleSwitch("ctrl:stage", StageOn, 296, 336, () => { this.StageLiftRequest = !StageOn; this.Dirty = true; });

            // Card 2: Thermostat (EXACT Image #2 Thermostat card with twin circular − / + buttons!)
            this.RasterRectangle(420, 128, 360, 268, 24, "#121316", "#202227", 1.1);
            this.RasterCircle(462, 174, 24, "#1c1e23");
            this.RasterGlyph("thermo", 462, 174, 18, "#e6ecf5");
            this.RasterText("Thermostat", 444, 260, 24, "#f0f3f8", "left", 400);
            this.RasterText(`${this.Values.temperature}°`, 444, 292, 18, "#9ba3b0", "left", 400);
            const MinusHover = this.Hovered === "ctrl:temp-";
            const PlusHover  = this.Hovered === "ctrl:temp+";
            this.RasterCircle(632, 344, 30, MinusHover ? "#272a31" : "#1b1d22", "#2b2e36");
            this.RasterGlyph("minus", 632, 344, 16, "#edf2f8");
            this.AdmitRegion("ctrl:temp-", 600, 312, 64, 64, () => this.IncrementValue("temperature", -1, 16, 28));
            this.RasterCircle(706, 344, 30, PlusHover ? "#272a31" : "#1b1d22", "#2b2e36");
            this.RasterGlyph("plus", 706, 344, 16, "#edf2f8");
            this.AdmitRegion("ctrl:temp+", 674, 312, 64, 64, () => this.IncrementValue("temperature", 1, 16, 28));

            // Card 3: Cabin Lock & Lighting (Pure White active card when unlocked/lit)
            this.RasterRectangle(798, 128, 360, 268, 24, this.Locked ? "#121316" : "#ffffff", this.Locked ? "#202227" : "#ffffff", 1.1);
            this.RasterCircle(840, 174, 24, this.Locked ? "#1c1e23" : "#eff2f6");
            this.RasterGlyph("lock", 840, 174, 18, this.Locked ? "#e6ecf5" : "#0b0c0e");
            this.RasterText("Vehicle Access", 822, 260, 24, this.Locked ? "#f0f3f8" : "#0b0c0e", "left", 500);
            this.RasterText(this.Locked ? "Secured · Cabin Locked" : "Unlocked · Ready to Drive", 822, 288, 14, this.Locked ? "#858d9b" : "#4a515d", "left", 400);
            this.RasterToggleSwitch("ctrl:lock", !this.Locked, 1052, 336, () => { this.Locked = !this.Locked; this.Dirty = true; });

            // Row 2: Live Tyre Pressure & Telemetry Summary Bento Strip
            const Pressures = Live.Pressures || [this.Values.pressure, this.Values.pressure, this.Values.pressure, this.Values.pressure];
            const Temps = Live.Temperatures || [28, 28, 27, 27];
            this.RasterRectangle(42, 418, 548, 270, 24, "#121316", "#202227", 1.1);
            this.RasterText("Chassis & Tyres", 70, 466, 22, "#f0f3f8", "left", 400);
            this.RasterText(`Target cold pressure · ${this.Values.pressure} kPa`, 70, 492, 13.5, "#7e8694", "left", 400);
            ["FL", "FR", "RL", "RR"].forEach((Corner, I) =>
            {
                const BX = 70 + I * 124, BY = 524;
                this.RasterRectangle(BX, BY, 112, 134, 16, "#181a1f", "#262931");
                this.RasterText(Corner, BX + 16, BY + 30, 12, "#7f8896", "left", 600);
                this.RasterText(`${Math.round(Pressures[I] ?? 220)}`, BX + 16, BY + 76, 26, "#eef2f8", "left", 400);
                this.RasterText("kPa", BX + 16, BY + 96, 11.5, "#6e7785", "left", 400);
                this.RasterText(`${Math.round(Temps[I] ?? 26)}°C`, BX + 16, BY + 120, 12.5, "#cde5dc", "left", 500);
            });

            // Right bottom card: Steering & Ride Height Steppers
            this.RasterRectangle(610, 418, 548, 270, 24, "#121316", "#202227", 1.1);
            this.RasterText("Dynamics & Calibration", 638, 466, 22, "#f0f3f8", "left", 400);
            this.RasterSlider("steering", "Steering Weight", "%", 638, 536, 488, 10, 100);
            this.RasterSlider("height",   "Ride Height",     " mm", 638, 622, 488, 10, 80);
            return;
        }

        // 5. GARAGE (Live 3D Car Configurator — obsidian minimal restyle preserving exact test hit coordinates!)
        if (Identifier === "garage")
        {
            const Preview = this.Preview;
            this.RasterRectangle(78, 248, 640, 400, 18, "#0f1115", "#242830", 1.2);
            if (Preview?.External)
            {
                this.RasterText("3D LIVE VEHICLE PREVIEW · DRAG TO ORBIT OR DROP MATERIALS", 96, 630, 11.5, "#7d8694", "left", 500);
            }
            else if (Preview?.Ready && Preview.Readback)
            {
                const Context = this.Context;
                Context.save();
                Context.translate(78, 648); Context.scale(1, -1);
                Context.drawImage(Preview.Readback, 0, 0, 640, 400);
                Context.restore();
            }
            else
            {
                this.RasterText("3D Preview Ready", 398, 452, 18, "#6f7886", "center", 400);
            }
            this.AdmitRegion("preview-orbit", 78, 248, 640, 400, null, { Orbit: true });

            this.RasterText("VEHICLE STUDIO", 762, 286, 11.5, "#7a8391", "left", 600);
            this.RasterText("Exterior Finish", 760, 336, 34, "#f2f5fa", "left", 400);
            this.RasterText("Tap a swatch or drag paint & rim finishes onto the vehicle.", 762, 372, 14, "#7e8795", "left", 400);
            this.RasterText("Body Coat", 762, 434, 13, "#b6bfcd", "left", 500);

            for (const [Index, Colour] of PaintColours.entries())
            {
                const Left = 762 + (Index % 2) * 128, Top = 462 + Math.floor(Index / 2) * 74;
                const Selected = this.FinishColour === Colour;
                this.RasterRectangle(Left, Top, 110, 58, 14, Colour, Selected ? "#ffffff" : "rgba(255,255,255,0.14)", Selected ? 2 : 1);
                this.AdmitRegion("colour:" + Index, Left, Top, 110, 58, () =>
                {
                    this.FinishColour = Colour;
                    this.Appearance?.Apply({ Colour });
                    this.Notice = "Finish updated live.";
                    this.Dirty = true;
                }, { Material: { Kind: "paint", Colour } });
            }

            this.RasterButton("finish", this.Appearance?.Matte ? "Matte Coat" : "Gloss Coat", 762, 606, 115, 36,
                () => { this.Appearance?.Apply({ Matte: !this.Appearance.Matte }); this.Dirty = true; });
            this.RasterButton("wheels", this.Appearance?.DarkWheels ? "Dark Rims" : "Alloy Rims", 885, 606, 115, 36,
                () => { this.Appearance?.Apply({ DarkWheels: !this.Appearance.DarkWheels, BlueRims: false }); this.Dirty = true; });
            const RimRegion = this.Regions.find(Region => Region.Identifier === "wheels");
            if (RimRegion) RimRegion.Material = { Kind: "wheel", DarkWheels: !this.Appearance?.DarkWheels, BlueRims: false };
            this.RasterButton("blue-rims", "Blue Rims", 1008, 606, 128, 36,
                () => { this.Appearance?.Apply({ DarkWheels: false, BlueRims: true }); this.Dirty = true; });
            const BlueRegion = this.Regions.find(R => R.Identifier === "blue-rims");
            if (BlueRegion) BlueRegion.Material = { Kind: "wheel", DarkWheels: false, BlueRims: true };
            const FinishRegion = this.Regions.find(R => R.Identifier === "finish");
            if (FinishRegion) FinishRegion.Material = { Kind: "finish", Matte: !this.Appearance?.Matte };

            this.RasterButton("inspection", `Mode: ${["Solid", "Ghost", "Wireframe", "Scan"][this.Appearance?.View || 0]}`, 762, 650, 238, 36,
                () => { if (this.Appearance) this.Appearance.View = (this.Appearance.View + 1) % 4; this.Dirty = true; });
            this.RasterButton("preview-spin", Preview?.Spin ? "Pause Turntable" : "Spin Turntable", 762, 694, 238, 36,
                () => { if (Preview) { Preview.Spin = !Preview.Spin; Preview.Dirty = true; this.Dirty = true; } }, "#121418", "#9ea7b5");
            return;
        }

        // 6. SYSTEM SETTINGS (Clean Minimal Display Brightness + System Diagnostics)
        this.RasterRectangle(42, 134, 1116, 210, 24, "#121316", "#202227", 1.1);
        this.RasterText("Display & System", 72, 186, 24, "#f0f3f8", "left", 400);
        this.RasterSlider("brightness", "HMI Display Brightness", "%", 72, 266, 1052, 0, 100);

        this.RasterRectangle(42, 368, 1116, 286, 24, "#121316", "#202227", 1.1);
        this.RasterText("Frontier Cabin OS · Minimal Dark Glass", 72, 420, 22, "#f0f3f8", "left", 400);
        this.RasterText("Zero-reload live scene switching enabled across all 5 Lobby Venues and 5 Driving Circuits.", 72, 452, 14.5, "#858d9b", "left", 400);
        this.RasterButton("sys:home", "Return to Map Console", 72, 560, 240, 46, () => this.PresentHome(), "#ffffff", "#0b0c0e", "#ffffff");
    }

    RasterSlider(Identifier, Label, Suffix, Left, Top, Width, Minimum, Maximum)
    {
        const Value = this.Values[Identifier];
        const Fraction = Math.max(0, Math.min(1, (Value - Minimum) / (Maximum - Minimum)));
        this.RasterText(Label, Left, Top - 18, 15, "#d8dfe9", "left", 400);
        this.RasterText(`${Value}${Suffix}`, Left + Width, Top - 18, 15, "#cde5dc", "right", 500);
        this.RasterRectangle(Left, Top, Width, 6, 3, "#20232a");
        this.RasterRectangle(Left, Top, Math.max(6, Width * Fraction), 6, 3, "#cde5dc");
        this.RasterCircle(Left + Width * Fraction, Top + 3, 10, "#ffffff", "#0d0f13", 2);
        this.AdmitRegion(`slider:${Identifier}`, Left - 12, Top - 18, Width + 24, 42, null,
            { Slider: Identifier, TrackLeft: Left, TrackWidth: Width, Minimum, Maximum, Step: 1 });
    }

    IncrementValue(Identifier, Difference, Minimum, Maximum)
    {
        this.Values[Identifier] = Math.max(Minimum, Math.min(Maximum, Math.round((this.Values[Identifier] + Difference) * 100) / 100));
        this.Dirty = true;
    }

    ReceiveText(Target)
    {
        this.TextTarget = Target;
        if (!this.ActiveTextInput) return;
        this.ActiveTextInput.value = Target === "message" ? this.MessageDraft : this.Nickname;
        this.ActiveTextInput.focus({ preventScroll: true });
        this.Dirty = true;
    }

    AdmitText(Value)
    {
        if (this.TextTarget === "message") this.MessageDraft = Value.slice(0, 100);
        else if (this.TextTarget === "nickname") this.Nickname = Value.slice(0, 22);
        this.Dirty = true;
    }

    DeliverMessage()
    {
        if (!this.MessageDraft.trim()) return;
        this.Messages.push({ From: "You", Text: this.MessageDraft.trim() });
        this.MessageDraft = "";
        if (this.ActiveTextInput) this.ActiveTextInput.value = "";
        this.Dirty = true;
    }

    RasterScreen()
    {
        if (!this.Dirty) return false;
        const Context = this.Context;
        Context.setTransform(this.Scale, 0, 0, this.Scale, this.OffsetAcross * this.Scale, this.OffsetDown * this.Scale);
        this.Regions = [];
        if (this.Application) this.RasterApplication();
        else this.RasterHomeConsole();

        if (this.DragMaterial && this.Pointer)
        {
            const M = this.DragMaterial, X = this.Pointer.PreviousAcross, Y = this.Pointer.PreviousDown;
            this.RasterRectangle(X - 66, Y - 24, 132, 48, 12, M.Colour || "#1d2736", "#ffffff", 1.5);
            this.RasterText(M.Kind === "paint" ? "Drop Paint" : M.Kind === "finish" ? "Drop Finish" : "Drop Rim", X, Y + 5, 13, "#ffffff", "center", 500);
        }
        if (!this.ExternalBrightness && this.Values.brightness < 100)
        {
            Context.fillStyle = `rgba(0, 0, 0, ${(100 - this.Values.brightness) / 100})`;
            Context.fillRect(-this.OffsetAcross, -this.OffsetDown, this.PanelWidth, this.PanelHeight);
        }
        ++this.Revision;
        this.Dirty = false;
        return true;
    }

    QueryContact(Across, Down)
    {
        return [...this.Regions].reverse().find(Region => Across >= Region.Left && Across <= Region.Left + Region.Width &&
            Down >= Region.Top && Down <= Region.Top + Region.Height) || null;
    }

    ForwardPointer(Phase, Across, Down)
    {
        const Contact = this.QueryContact(Across, Down);
        if (Phase === "hover")
        {
            const Identifier = Contact?.Identifier || "";
            if (Identifier !== this.Hovered) { this.Hovered = Identifier; this.Dirty = true; }
            return Boolean(Contact);
        }
        if (Phase === "down")
        {
            this.Pointer = { Contact, Across, Down, LastAcross: Across, PreviousAcross: Across, PreviousDown: Down, Moved: false };
            this.Pressed = Contact?.Identifier || "";
            if (Contact?.Slider) this.AdvanceSlider(Contact, Across);
            this.Dirty = true;
            return true;
        }
        if (Phase === "move" && this.Pointer)
        {
            const Active = this.Pointer;
            Active.LastAcross = Across;
            if (Math.hypot(Across - Active.Across, Down - Active.Down) > 9) Active.Moved = true;
            if (Active.Contact?.Material && Active.Moved)
            {
                if (!this.DragMaterial && this.Preview) { Active.PriorSpin = this.Preview.Spin; this.Preview.Spin = false; }
                this.DragMaterial = Active.Contact.Material;
                this.Dirty = true;
            }
            if (Active.Contact?.Slider) this.AdvanceSlider(Active.Contact, Across);
            if (Active.Contact?.Orbit && this.Preview) { this.Preview.Orbit((Across - Active.PreviousAcross) * 0.012, (Down - Active.PreviousDown) * 0.008); this.Dirty = true; }
            Active.PreviousAcross = Across; Active.PreviousDown = Down;
            return true;
        }
        if (Phase === "up" && this.Pointer)
        {
            const Active = this.Pointer;
            if (Active.Contact?.Material && Active.Moved)
            {
                const Inside = Across >= 78 && Across <= 718 && Down >= 248 && Down <= 648;
                const Pick = Inside ? this.Preview?.Pick?.((Across - 78) / 640, (Down - 248) / 400) : null;
                const Material = Active.Contact.Material;
                const Valid = Material.Kind !== "wheel" ? Pick?.Family === CourseMaterial.BodyCoat
                    : Pick?.Occupant > 0 && [CourseMaterial.Hub, CourseMaterial.RimBase].includes(Pick?.Family);
                if (Valid)
                {
                    this.Appearance?.Apply(Material.Kind === "paint" ? { Colour: Material.Colour } : Material.Kind === "finish" ? { Matte: Material.Matte } : { DarkWheels: Material.DarkWheels, BlueRims: Material.BlueRims });
                    if (Material.Colour) this.FinishColour = Material.Colour;
                }
                this.DragMaterial = null;
            }
            else if (!Active.Moved && Active.Contact && Contact?.Identifier === Active.Contact.Identifier)
            {
                Active.Contact.Action?.();
            }
            if (Active.PriorSpin !== undefined && this.Preview) this.Preview.Spin = Active.PriorSpin;
            this.Pointer = null;
            this.Pressed = "";
            this.Dirty = true;
            return true;
        }
        if (Phase === "cancel")
        {
            if (this.Pointer?.PriorSpin !== undefined && this.Preview) this.Preview.Spin = this.Pointer.PriorSpin;
            this.DragMaterial = null;
            this.Pointer = null;
            this.Pressed = "";
            this.Dirty = true;
        }
        return false;
    }

    AdvanceSlider(Contact, Across)
    {
        const Fraction = Math.max(0, Math.min(1, (Across - Contact.TrackLeft) / Contact.TrackWidth));
        this.Values[Contact.Slider] = Math.round((Contact.Minimum + Fraction * (Contact.Maximum - Contact.Minimum)) / Contact.Step) * Contact.Step;
        this.Dirty = true;
    }

    QueryReading()
    {
        return {
            Design: this.Design, Application: this.Application, Revision: this.Revision,
            ApplicationCount: Applications.length, Values: { ...this.Values }, RoomReady: this.RoomReady,
            Nickname: this.Nickname, MessageCount: this.Messages.length, FocusIndex: this.FocusIndex,
            TextTarget: this.TextTarget, RegionCount: this.Regions.length
        };
    }
}
