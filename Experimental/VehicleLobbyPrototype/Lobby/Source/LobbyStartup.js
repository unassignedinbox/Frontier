//====================================================================================================================================
//                                                       LOBBYSTARTUP.JS
//====================================================================================================================================
// 🧩 Lobby page entry: writes the studio environment, starts the VehicleHMI host, then mounts the lobby overlay.

// 📝 Environment for the studio. Every sky, sun, cloud and fog switch is off so that only the softbox lights the car.
//    The keys are the renderer's own (EnvironmentPersistence.js StorageKey and EnvironmentSettings defaults).
const EnvironmentStorageKey = "Frontier.Environment.v1";
const BackupStorageKey = "Frontier.Lobby.EnvironmentBackup";
const StudioEnvironment = Object.freeze({
    AtmosphereEnabled: false, SunEnabled: false, StarsEnabled: false, MoonEnabled: false, PlanetsEnabled: false,
    CloudEnabled: false, FogEnabled: false, FlareEnabled: false, AerialEnabled: false, MoonLightEnabled: false,
    LocalFogEnabled: false
});

// 📝 Backs up the environment the level editor saved, once. Travel restores it, so the next level keeps its own settings.
function StoreStudioEnvironment() {
    const Current = localStorage.getItem(EnvironmentStorageKey);
    const Studio = JSON.stringify(StudioEnvironment);
    if (Current === Studio)
        return;
    if (localStorage.getItem(BackupStorageKey) === null)
        localStorage.setItem(BackupStorageKey, Current ?? "");
    localStorage.setItem(EnvironmentStorageKey, Studio);
}

StoreStudioEnvironment();

// 📝 Driving keys must not move the car in the lobby. The capture-phase listener stops them before the host sees them.
const DrivingKeys = new Set(["KeyW", "KeyA", "KeyS", "KeyD", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "Space", "ShiftLeft"]);
window.addEventListener("keydown", (Event) => {
    if (DrivingKeys.has(Event.code) && !(Event.target instanceof HTMLInputElement))
        Event.stopPropagation();
}, true);

// 📝 The host self-starts on import and publishes window.IlluminationExhibit once the first frame is up.
import("/Source/IlluminationHost.js?v=lighting-training-2")
    .then(() => import("./LobbyWorkspace.js"))
    .catch((Error0) => {
        console.error("Lobby startup failed:", Error0);
        document.getElementById("bring-up")?.classList.add("hidden");
        const Panel = document.getElementById("failure");
        if (Panel) {
            Panel.classList.remove("hidden");
            Panel.textContent = "Could not start the lobby. " + String(Error0?.message ?? Error0) + " — check the preview server, then reload.";
        }
    });
