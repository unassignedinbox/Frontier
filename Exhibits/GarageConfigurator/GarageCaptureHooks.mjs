// Capture driver: give the headless harness just enough canvas/Image for the HMI to construct,
// open the Garage, and pin the paint ripple mid-spread. The 2D panel texture upload is stubbed
// (a fake canvas is not a GPU image source); the 3D preview well is the real ForwardRaster pass.
function MakeContext(Canvas) {
    const Gradient = { addColorStop() {} };
    const Store = { canvas: Canvas };
    return new Proxy(Store, {
        get(Target, Key) {
            if (Key in Target) return Target[Key];
            if (Key === "measureText") return () => ({ width: 12 });
            if (Key === "createLinearGradient" || Key === "createRadialGradient") return () => Gradient;
            if (Key === "getImageData") return () => ({ data: new Uint8ClampedArray(4), width: 1, height: 1 });
            return () => undefined;
        },
        set(Target, Key, Value) { Target[Key] = Value; return true; }
    });
}
const OriginalCreate = document.createElement.bind(document);
document.createElement = (Tag) => {
    if (Tag !== "canvas") return OriginalCreate(Tag);
    return { width: 300, height: 150, style: {}, addEventListener() {}, removeEventListener() {},
             getContext(Kind) { if (Kind !== "2d") return null; if (!this._Context) this._Context = MakeContext(this); return this._Context; } };
};
globalThis.Image = class { set src(_Value) { queueMicrotask(() => this.onload && this.onload()); } };

export default {
    Begun: false,
    async BeforeFrame(Host, Frame) {
        if (!Host.Cabin) return;
        const Cabin = Host.Cabin;
        if (Cabin.Screen && !this.Stubbed) {
            this.Stubbed = true;
            Cabin.Screen.RasterScreen = () => false;
            if (Cabin.Cluster) Cabin.Cluster.RasterCluster = () => false;
        }
        if (!Cabin.Screen || Frame < 1) return;
        Host.AssignCameraMode("HMI");
        if (Host.LobbyBlend) Host.LobbyBlend.Seconds = 999;
        if (Cabin.Screen.Application !== "garage") Cabin.Screen.PresentApplication("garage");
        const Appearance = Cabin.Appearance;
        if (!Appearance) return;
        if (!this.Begun) {
            this.Begun = true;
            const Set = Host.Scene.FacetSets[Host.Scene.VehicleBodySet];
            Appearance.BeginPaintRipple("#8a1a1d",
                [Set.LocalMaximum[0], 0, 0.5 * (Set.LocalMinimum[2] + Set.LocalMaximum[2])]);
        }
        if (Appearance.Ripple.Active) Appearance.Ripple.Elapsed = 0.60;
    },
    async Report(Host) {
        console.log("DEBUG mode=", Host.Settings?.CameraMode, "app=", Host.Cabin?.Screen?.Application,
            "hmi=", Host.Cabin?.Metrics?.Hmi, "rippleActive=", Host.Cabin?.Appearance?.Ripple?.Active,
            "radius=", Host.Cabin?.Appearance?.Ripple?.Radius, "previewPasses=", Host.Cabin?.Metrics?.PreviewPasses);
    }
};
