// Seamless Embedded Workspace Host for:
//   1. Component Inspector (Experimental/ComponentViewer/index.html?host=frontier-editor)
//      Opens like a Blueprint editor when inspecting any object in the Frontier GI World.
//   2. Geometry Compile (Experimental/GeometryCompiler/Compile.html?host=frontier-editor)
//      Opens like an Asset Import Menu & 8-Step Asset Inspector, compiling meshes/UV/SDF/LOD/Collision/Icon
//      and delivering them straight into Frontier Editor's Content Browser & Global Illumination World.

import React, { useEffect, useRef, useState } from "react";

const CV_CHANNEL = "frontier-component-viewer";
const GC_CHANNEL = "frontier-geometry-compile";

function ResolveStudioUrl(RelativeFromExperimental) {
  if (typeof window === "undefined") return `/${RelativeFromExperimental}`;
  const Path = window.location.pathname || "/";
  if (Path.includes("/FrontierWorldMirror")) {
    return `../${RelativeFromExperimental}`;
  }
  return `/${RelativeFromExperimental}`;
}

// Parse an uploaded browser File object into a Geometry Compile asset payload
async function ParseFileForGeometryCompile(File) {
  const Name = String(File?.name || "ImportedModel.obj");
  const Ext = Name.split(".").pop().toLowerCase();
  const Buffer = await File.arrayBuffer();
  const BaseName = Name.replace(/\.[^.]+$/, "");
  // Let GeometryCompiler parse the file natively if possible, or hand raw bytes via File fallback
  return { fileName: Name, ext: Ext, name: BaseName, fileSize: File.size, buffer: Buffer };
}

export default function StudioWorkspacesModal({
  ActiveStudio,          // null | 'component-viewer' | 'geometry-compile'
  InspectPayload,        // { entity, compiledAsset, compiledAssets, nonce }
  CompilePayload,        // { mode: 'start' | 'asset' | 'file', asset?, file?, recompile?, nonce }
  CompiledAssets,        // array of compiled assets from session
  OnCloseStudio,
  OnComponentSync,       // ({ entityId, blueprintName, wingVisible, headlightsOn, componentCount, close })
  OnCompiledAssetReady,  // (asset, { recompile, openInWorld })
}) {
  const CvFrameRef = useRef(null);
  const GcFrameRef = useRef(null);
  const [CvMounted, SetCvMounted] = useState(false);
  const [GcMounted, SetGcMounted] = useState(false);
  const CvReadyRef = useRef(false);
  const GcReadyRef = useRef(false);
  const PendingInspectRef = useRef(null);
  const PendingCompileRef = useRef(null);

  // Lazy-mount each studio iframe on first open, then keep alive for instant re-opening
  useEffect(() => {
    if (ActiveStudio === "component-viewer" && !CvMounted) SetCvMounted(true);
    if (ActiveStudio === "geometry-compile" && !GcMounted) SetGcMounted(true);
  }, [ActiveStudio, CvMounted, GcMounted]);

  // Listen for postMessage handshakes from ComponentViewer and GeometryCompiler
  useEffect(() => {
    const OnMessage = (Ev) => {
      const Msg = Ev.data;
      if (!Msg || typeof Msg !== "object") return;

      // Component Inspector channel
      if (Msg.channel === CV_CHANNEL) {
        if (Msg.type === "ready") {
          CvReadyRef.current = true;
          if (PendingInspectRef.current && CvFrameRef.current?.contentWindow) {
            CvFrameRef.current.contentWindow.postMessage(
              { channel: CV_CHANNEL, type: "inspect-entity", ...PendingInspectRef.current },
              "*",
            );
            PendingInspectRef.current = null;
          }
        } else if (Msg.type === "sync") {
          OnComponentSync?.(Msg.summary || {}, Boolean(Msg.close));
          if (Msg.close) OnCloseStudio?.();
        } else if (Msg.type === "asset-compiled" && Msg.asset) {
          OnCompiledAssetReady?.(Msg.asset, { recompile: false, openInWorld: false });
        }
        return;
      }

      // Geometry Compiler channel (CompileExchange.js protocol)
      if (Msg.channel === GC_CHANNEL) {
        if (Msg.type === "ready") {
          GcReadyRef.current = true;
          if (PendingCompileRef.current && GcFrameRef.current?.contentWindow) {
            DispatchCompileMessage(GcFrameRef.current.contentWindow, PendingCompileRef.current);
            PendingCompileRef.current = null;
          }
        } else if (Msg.type === "finish") {
          if (!Msg.cancelled && Msg.asset) {
            OnCompiledAssetReady?.(Msg.asset, {
              recompile: Boolean(Msg.existing),
              openInWorld: Boolean(Msg.open),
            });
          }
          OnCloseStudio?.();
        }
      }
    };
    window.addEventListener("message", OnMessage);
    return () => window.removeEventListener("message", OnMessage);
  }, [OnCloseStudio, OnComponentSync, OnCompiledAssetReady]);

  // Push inspect-entity request whenever InspectPayload updates
  useEffect(() => {
    if (ActiveStudio !== "component-viewer" || !InspectPayload) return;
    const Payload = {
      entity: InspectPayload.entity,
      compiledAsset: InspectPayload.compiledAsset || null,
      compiledAssets: CompiledAssets || [],
    };
    if (CvReadyRef.current && CvFrameRef.current?.contentWindow) {
      CvFrameRef.current.contentWindow.postMessage(
        { channel: CV_CHANNEL, type: "inspect-entity", ...Payload },
        "*",
      );
    } else {
      PendingInspectRef.current = Payload;
    }
  }, [ActiveStudio, InspectPayload, CompiledAssets]);

  // Push Geometry Compile open request whenever CompilePayload updates
  useEffect(() => {
    if (ActiveStudio !== "geometry-compile" || !CompilePayload) return;
    if (GcReadyRef.current && GcFrameRef.current?.contentWindow) {
      DispatchCompileMessage(GcFrameRef.current.contentWindow, CompilePayload);
    } else {
      PendingCompileRef.current = CompilePayload;
    }
  }, [ActiveStudio, CompilePayload]);

  // Escape key closes modal overlay when user presses Shift+Esc or clicks top-bar back
  const CvVisible = ActiveStudio === "component-viewer";
  const GcVisible = ActiveStudio === "geometry-compile";
  const AnyVisible = CvVisible || GcVisible;

  return (
    <div
      className={"frontier-studio-modal-backdrop" + (AnyVisible ? " open" : "")}
      aria-hidden={!AnyVisible}
      style={{
        position: "fixed",
        inset: 0,
        zIndex: 90,
        display: AnyVisible ? "flex" : "none",
        flexDirection: "column",
        background: "rgba(9, 10, 13, 0.96)",
        backdropFilter: "blur(10px)",
      }}
    >
      {/* Slim Frontier Editor Workspace Switch Bar */}
      <div
        className="frontier-studio-modal-bar"
        style={{
          height: 36,
          flexShrink: 0,
          display: "flex",
          alignItems: "center",
          gap: 10,
          padding: "0 12px",
          background: "#14161a",
          borderBottom: "1px solid rgba(255,255,255,0.10)",
          color: "#e6edf5",
          fontSize: 12,
        }}
      >
        <button
          type="button"
          onClick={OnCloseStudio}
          style={{
            display: "inline-flex",
            alignItems: "center",
            gap: 6,
            padding: "4px 10px",
            borderRadius: 6,
            border: "1px solid rgba(246, 198, 107, 0.42)",
            background: "rgba(246, 198, 107, 0.14)",
            color: "#f6c66b",
            fontSize: 11.5,
            fontWeight: 600,
            cursor: "pointer",
          }}
        >
          ← Return to GI World
        </button>
        <span style={{ opacity: 0.4 }}>|</span>
        <strong style={{ fontSize: 12, fontWeight: 600, letterSpacing: 0.2 }}>
          {CvVisible
            ? `Component Inspector · ${InspectPayload?.entity?.Name || "BP_ControlVehicle"}`
            : "Geometry Compile · Asset Import & 8-Step Compiler"}
        </strong>
        <span
          style={{
            fontSize: 10.5,
            padding: "2px 7px",
            borderRadius: 999,
            background: CvVisible ? "rgba(96, 165, 250, 0.16)" : "rgba(52, 211, 153, 0.16)",
            border: CvVisible ? "1px solid rgba(96, 165, 250, 0.36)" : "1px solid rgba(52, 211, 153, 0.36)",
            color: CvVisible ? "#93c5fd" : "#6ee7b7",
          }}
        >
          {CvVisible ? "Blueprint Inspector" : "Asset Pipeline"}
        </span>
        <span style={{ flex: 1 }} />
        <span style={{ fontSize: 11, opacity: 0.6 }}>
          {CvVisible
            ? "Inspector Viewport (Standalone) · Global Illumination runs in World Viewport"
            : "Compiles Source, Normals, UV Charts, SDF, Cluster LOD, Collision & Icon into Frontier World"}
        </span>
        <button
          type="button"
          onClick={OnCloseStudio}
          title="Close workspace"
          style={{
            width: 24,
            height: 24,
            borderRadius: 6,
            border: "1px solid rgba(255,255,255,0.12)",
            background: "rgba(255,255,255,0.05)",
            color: "#cbd5e1",
            cursor: "pointer",
            fontSize: 13,
            lineHeight: 1,
          }}
        >
          ×
        </button>
      </div>

      {/* Embedded Component Inspector iframe */}
      {CvMounted && (
        <iframe
          ref={CvFrameRef}
          title="Frontier Component Inspector"
          src={ResolveStudioUrl("ComponentViewer/index.html?host=frontier-editor")}
          style={{
            flex: 1,
            width: "100%",
            height: "100%",
            border: 0,
            display: CvVisible ? "block" : "none",
            background: "#0b0b0d",
          }}
        />
      )}

      {/* Embedded Geometry Compiler iframe */}
      {GcMounted && (
        <iframe
          ref={GcFrameRef}
          title="Frontier Geometry Compile Asset Inspector"
          src={ResolveStudioUrl("GeometryCompiler/Compile.html?host=frontier-editor")}
          style={{
            flex: 1,
            width: "100%",
            height: "100%",
            border: 0,
            display: GcVisible ? "block" : "none",
            background: "#0b0b0d",
          }}
        />
      )}
    </div>
  );
}

function DispatchCompileMessage(TargetWindow, Payload) {
  if (!TargetWindow || !Payload) return;
  if (Payload.mode === "asset" && Payload.asset) {
    TargetWindow.postMessage(
      {
        channel: GC_CHANNEL,
        type: "open",
        asset: Payload.asset,
        recompile: Boolean(Payload.recompile),
        startOnly: false,
      },
      "*",
    );
    return;
  }
  if (Payload.mode === "file" && Payload.file) {
    // Open start screen and forward file into GeometryCompiler's FileInput or drag handler
    TargetWindow.postMessage(
      {
        channel: GC_CHANNEL,
        type: "open",
        asset: null,
        recompile: false,
        startOnly: true,
      },
      "*",
    );
    // Also trigger native file parsing via same-origin window helper if exposed
    try {
      const FileInput = TargetWindow.document?.getElementById("FileInput");
      if (FileInput && typeof DataTransfer !== "undefined") {
        const Dt = new DataTransfer();
        Dt.items.add(Payload.file);
        FileInput.files = Dt.files;
        FileInput.dispatchEvent(new Event("change", { bubbles: true }));
      }
    } catch {}
    return;
  }
  // Default: open the Geometry Compile Start / Import / Samples screen
  TargetWindow.postMessage(
    {
      channel: GC_CHANNEL,
      type: "open",
      asset: null,
      recompile: false,
      startOnly: true,
    },
    "*",
  );
}

export { ParseFileForGeometryCompile };
