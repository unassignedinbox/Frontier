import { test } from "node:test";
import assert from "node:assert/strict";

import {
    CreateTyreModel, TyreModelKinds, TyreModelDiagnostics, TyreReferenceCompression, TyreReferencePa, AirStiffness,
    TyreReferenceSidewall
} from "../Source/TyreModels.js";
import { VehicleSolver } from "../Source/VehicleSolver.js";

const Spring = CreateTyreModel(TyreModelKinds.Spring);
const Xpbd = CreateTyreModel(TyreModelKinds.Xpbd);
const Fem = CreateTyreModel(TyreModelKinds.Fem);
const SpringAtReference = Spring.Force(TyreReferenceCompression, TyreReferencePa);

test("the spring model is the closed-form sidewall plus air law the exhibit was tuned with", () => {
    for (const Compression of [0.0, 0.004, 0.014, 0.03]) {
        const Expected = (TyreReferenceSidewall + AirStiffness(TyreReferencePa)) * Compression;
        assert.ok(Math.abs(Spring.Force(Compression, TyreReferencePa) - Expected) < 1e-9);
    }
    assert.equal(Spring.Force(0.0, TyreReferencePa), 0.0);
    assert.ok(Spring.Force(0.01, 260000) > Spring.Force(0.01, 180000), "more pressure, more stiffness");
});

test("XPBD and FEM are calibrated to the spring's sag at the reference compression", () => {
    const Xs = Xpbd.Force(TyreReferenceCompression, TyreReferencePa);
    const Fs = Fem.Force(TyreReferenceCompression, TyreReferencePa);
    assert.ok(Math.abs(Xs - SpringAtReference) / SpringAtReference < 0.05, `XPBD ${Xs.toFixed(0)} N vs spring ${SpringAtReference.toFixed(0)} N`);
    assert.ok(Math.abs(Fs - SpringAtReference) / SpringAtReference < 0.001, `FEM ${Fs.toFixed(0)} N vs spring ${SpringAtReference.toFixed(0)} N`);
    assert.ok(TyreModelDiagnostics.XpbdCalibration(), "the XPBD table recorded its calibration");
    assert.ok(TyreModelDiagnostics.FemModulus() > 0, "the FEM modulus was calibrated to a positive value");
});

test("every model's radial force is zero unloaded and rises monotonically with compression", () => {
    for (const Model of [Spring, Xpbd, Fem]) {
        assert.equal(Model.Force(0.0, TyreReferencePa), 0.0, `${Model.Kind} unloaded`);
        let Previous = -Infinity;
        for (let I = 0; I <= 20; ++I) {
            const F = Model.Force(I * 0.0025, TyreReferencePa);
            assert.ok(Number.isFinite(F), `${Model.Kind} finite`);
            assert.ok(F >= Previous - 1e-6, `${Model.Kind} monotone at step ${I}`);
            Previous = F;
        }
    }
});

test("the XPBD and FEM responses differ in shape from the spring, not just in level", () => {
    // Normalised to each model's own sag, the spring is perfectly linear; the carcass models are not.
    const Ratio = (Model, Compression) => Model.Force(Compression, TyreReferencePa) / (Model.Force(TyreReferenceCompression, TyreReferencePa) * Compression / TyreReferenceCompression);
    const Shallow = 0.003, Deep = 0.03;
    const SpringShape = Ratio(Spring, Shallow) / Ratio(Spring, Deep);
    const XpbdShape = Ratio(Xpbd, Shallow) / Ratio(Xpbd, Deep);
    assert.ok(Math.abs(SpringShape - 1.0) < 1e-9);
    assert.ok(Math.abs(XpbdShape - 1.0) > 0.05, `XPBD stiffens with its contact patch (shape ${XpbdShape.toFixed(3)})`);
});

test("pressure stiffens the carcass in every model", () => {
    const Low = 180000, High = 260000;
    assert.ok(Spring.Force(0.012, High) > Spring.Force(0.012, Low));
    assert.ok(Fem.Force(0.012, High) > Fem.Force(0.012, Low));
    assert.ok(Xpbd.Force(0.012, High) > Xpbd.Force(0.012, Low), "the pressure load widens the XPBD contact patch");
});

test("the FEM operator is symmetric and carries no force under rigid translation", () => {
    const { K, Dof } = TyreModelDiagnostics.FemOperator();
    let Asymmetry = 0.0, Scale = 0.0;
    for (let I = 0; I < Dof; ++I)
        for (let J = 0; J < Dof; ++J) {
            Asymmetry = Math.max(Asymmetry, Math.abs(K[I * Dof + J] - K[J * Dof + I]));
            Scale = Math.max(Scale, Math.abs(K[I * Dof + J]));
        }
    assert.ok(Asymmetry <= 1e-9 * Scale, "symmetric");
    for (let Axis = 0; Axis < 3; ++Axis) {
        let Residual = 0.0;
        for (let I = 0; I < Dof; ++I) {
            let Sum = 0.0;
            for (let J = Axis; J < Dof; J += 3) Sum += K[I * Dof + J];
            Residual = Math.max(Residual, Math.abs(Sum));
        }
        assert.ok(Residual <= 1e-8 * Scale, `translation along axis ${Axis} is force-free`);
    }
});

test("the vehicle settles on each carcass model without NaNs and with a physical sag", () => {
    for (const Kind of [TyreModelKinds.Spring, TyreModelKinds.Xpbd, TyreModelKinds.Fem]) {
        const Vehicle = new VehicleSolver([]);
        Vehicle.SetTyreModel(Kind);
        assert.equal(Vehicle.TyreModel.Kind, Kind);
        Vehicle.Respawn([-100, -34, 0.53], 0);
        for (let I = 0; I < 90; ++I) Vehicle.Advance({ Throttle: 0, Brake: 0, Steer: 0, Handbrake: false }, 1 / 120);
        for (const Wheel of Vehicle.Wheels) {
            assert.ok(Number.isFinite(Wheel.TyreDeflection) && Number.isFinite(Wheel.TyreForce), `${Kind} finite`);
            assert.ok(Wheel.TyreDeflection > 0.004 && Wheel.TyreDeflection < 0.03, `${Kind} sag ${Wheel.TyreDeflection}`);
        }
    }
});
