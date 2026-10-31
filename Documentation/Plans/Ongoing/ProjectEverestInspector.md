# Project Everest — modular inspector model

## TL;DR

A record's constructor registers it in the Outliner and hands over an editor-only description that draws its own
inspector page; its destructor withdraws both. Engine-reusable records (Sun, sky, atmosphere, clouds, fog, wind,
Moon, stars, lens optics) ship their pages with the engine, so a user never re-authors them. Authored records (a
vehicle, a fracture target, a plume emitter) are created and described by the project. Pages draw through one
canvas, so a record never touches ImGui, and Shipping contains neither pages nor canvas.

## 1. The two kinds of record

| Kind | `RecordProvisioning` | Constructed by | Inspector page | Outliner |
|------|----------------------|----------------|----------------|----------|
| Engine-reusable | `EngineReusable` | The scene division inserts one per scene at bring-up | Ships with the engine; user edits values only | Present, not deletable |
| Project-authored | `ProjectAuthored` | Project code, at authoring time | Written by the project that owns the record; may be a preset page, a custom page, or a studio | Present, deletable |
| Derived | `Derived` | Owner record, e.g. a tyre set inside a vehicle | Owned by the same author as its owner; nests under it | Nested, not separately deletable |

💡 The distinction is the owner's real distinction: nobody redesigns the Sun, everybody designs their own vehicle.
It is a property of the record, not a convention a project can forget.

## 2. Registration and withdrawal

```cpp
namespace Frontier
{
    RecordEntry::RecordEntry(RecordClassification EntryClassification)
        : IssuedToken(TokenIssuer::Issue())
        , EntryClassification(EntryClassification)
    {
        RecordStore::Insert(*this);
#if defined(FRONTIER_EDITOR_PROFILE)
        SceneDirectory::Register(*this);          // Outliner row appears
        InspectorPageRegistry::Register(*this, BuildInspectorProfile());  // page appears with it
#endif
    }

    RecordEntry::~RecordEntry()
    {
#if defined(FRONTIER_EDITOR_PROFILE)
        InspectorPageRegistry::Withdraw(*this);   // selection cleared, studios closed, no dangling fields
        SceneDirectory::Unregister(*this);
#endif
        RecordStore::Remove(*this);
    }
}
```

- ⚠️ Ordering matters: `SceneDivision` constructs the directory before any record exists. Static-initialisation
  order across the project DLL boundary is a build-time check, not an assumption.
- ⚠️ Threads: records are constructed on the main thread in editor builds. Jolt job threads never construct a
  record; physics results land on existing ones.
- The description is built by the record, so a record that forgets to describe itself is a compile-time absence of
  `BuildInspectorProfile()`, not a runtime blank page.

## 3. The description a record hands over

```text
RecordInspectorProfile                     # editor-only; absent from Shipping
├── StableId          [text]  Pinned HTML id, e.g. "sun", "vehicle"
├── Identity          name · subtitle · classification · enabled-pill text
├── GroupToken        Outliner group: Render · Environment · Forcefield · Objects · Vehicle · Effects ·
│                     Destruction · Level
├── Provisioning      EngineReusable · ProjectAuthored · Derived
├── PageToken         Which page family draws it
├── StudioToken       Optional dedicated studio; empty for reusable records
├── Infographics      Optional CountTile / Metric / Sparkline set (SourceImages/General UI statistical panels)
└── Build(InspectorCanvas&, RecordEntry&)   The record's own page body
```

Every control the page builds writes **through a pointer to the live setting in the record**. There is no mirrored
configuration object, so an edit cannot drift from the value the renderer reads.

## 4. The canvas

```cpp
// 📝 One drawing surface for every page. Records never include imgui.h; the toolkit stays replaceable and
//    Shipping links no page code at all.
struct InspectorCanvas
{
    void BeginCard(const char *Title);
    void EndCard();

    void ScalarField(RecordEditorScalarField &Field);      // label · unit · range captions · number field
    void Switch(RecordEditorSwitch &Switch);               // .property-switch
    void Choice(RecordEditorChoiceField &Field);           // segmented pills or radio
    void Colour(RecordEditorColourField &Field);           // .colour-pair + .colour-code
    void Material(RecordEditorMaterialField &Field);       // .material-chip + swatch
    void Metric(const char *Label, const char *Unit, double Reading, const char *Delta);
    void CountTile(RecordEditorCountTile &Tile);
    void Sparkline(const float *Samples, size_t SampleCount, float Threshold);
    void Action(RecordEditorAction &Action);               // .action-button (+ primary / danger)
    void Text(const char *Caption, const char *Reading);
    void Divider();
};
```

Geometry stays 1:1 because the canvas owns it: a page chooses *which* control, never *how big* it is. That is the
mechanism that lets thirteen page files share one visual contract with the demo's CSS.

## 5. Who owns which page

| Family | Owner | Page file |
|--------|-------|-----------|
| Sun, sky, atmosphere, clouds, fog, wind, Moon, stars, lens optics, environment root | Engine (`EngineReusable`) | `UI/Editor/InspectorPages/EnvironmentInspectorPages.cpp` |
| GI, ReSTIR, reflection, quality, display, exposure, denoiser | Engine (`EngineReusable`, bound to the renderer) | `RenderInspectorPages.cpp` |
| Diagnostics, telemetry, hardware | Engine (`EngineReusable`) | `DiagnosticsInspectorPages.cpp` |
| Vehicle systems, tyres, drivetrain, cabin | Project (`ProjectAuthored`) | `VehicleInspectorPages.cpp` |
| Plume, sparks, precipitation, snow, water, fracture | Mixed: emitters are project-authored, precipitation and snow surfaces are engine-reusable | `EffectInspectorPages.cpp` |
| Materials, textures, patterns | Engine, reusing Texture Studio's kit and archives | existing Texture Studio panels |

A project that wants a different vehicle page overrides its own `BuildInspectorProfile()`; it cannot override an
engine-reusable one, and it never has to.

## 6. Dedicated studios

Some families deserve a whole window, not a column: fracture, pyro plume, particles, cabin HMI, vehicle, material.
These are `DedicatedRecordStudios` (renamed from the donor's banned `DedicatedEntityEditors`).

| Property | Rule |
|----------|------|
| Binding | Zero-copy against the live record; no duplicated record memory, asserted by address comparison |
| Lifetime | Opened from the page's `Action` or the Outliner context menu; closed when its record dies |
| Content | Preset bank (nine for plume), preview render, family-specific instrument cluster |
| Profile | Editor only; absent from Shipping, proved by symbols |
| Nesting | A studio is a workspace, not a dock panel; it never blocks the drawer's pointer ownership |

## 7. Constraints and checks

| Check | Method |
|-------|--------|
| Registration follows construction and destruction | Construct and destroy a record; the Outliner row and page both appear and disappear |
| No dangling page after deletion | Delete the selected record mid-edit; selection clears, no field pointer survives |
| Shipping carries no editor machinery | `nm` on the Shipping binary shows zero `SceneDirectory`, `InspectorCanvas`, `InspectorPage` symbols |
| Enabled state matches the scene | Toggling the eye changes actual visibility, not only an icon |
| Reusable records cannot be deleted | The delete action is absent for `EngineReusable` rows |
| Derived records nest and die with their owner | Destroy a vehicle; its tyre set leaves the Outliner and its page closes |
| Edits reach the renderer | Change a value, read the renderer configuration back, observe the rendered result change |

## 8. Files

| Path | Purpose |
|------|---------|
| `Engine/Scene/RecordInspectorProfile.{h,cpp}` | Description, provisioning, field structs |
| `Engine/Scene/RecordProvisioning.h` | The three-way classification |
| `Engine/Interface/InspectorCanvas.{h,cpp}` | Drawing surface |
| `Engine/Interface/InspectorPageRegistry.{h,cpp}` | Register, withdraw, resolve by classification |
| `UI/Editor/InspectorPages/*.cpp` | Five page files of §5 |
| `UI/Editor/DedicatedRecordStudios.{h,cpp}` | Studio host |
| `Exhibits/EverestInspector/` | Validation and captures |

⚠️ The donor's `RecordEditorDescription` is upgraded in place rather than replaced, because its flat field list is
already exercised by `EnvironmentRecords`. The upgrade adds provisioning, page and studio tokens plus the
`Build` callback; it does not fork a second description type.
