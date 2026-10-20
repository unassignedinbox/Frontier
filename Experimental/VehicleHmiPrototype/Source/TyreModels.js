//============================================================================================================================================
//                                                           TYREMODELS.JS
//============================================================================================================================================
// 🧩 Three radial tyre models behind one interface: the carcass force the contact solver balances for a given compression.
//      • Spring — the closed-form spring-in-series law the exhibit has always used (sidewall + air stiffness).
//      • XPBD   — a ring of compliant carcass particles relaxed with position-based dynamics: edges, spokes to the rim, the
//                 pressure load, and the road as a unilateral constraint. The contact patch, and so the force, grows with
//                 the compression.
//      • FEM    — a 3D linear tetrahedral block of tyre section, assembled once; per compression, the contact patch is
//                 pinned to the road, the rim face is displaced, and the free nodes are condensed out.

// 📝 Calibration. Every model reports the same static radial force at the reference compression and reference pressure,
//    so switching models changes the shape of the response (patch growth, pressure stiffening) and never the tuning.
export const TyreModelKinds = Object.freeze({ Spring: "Spring", Xpbd: "XPBD", Fem: "FEM" });
export const TyreReferenceCompression = 0.014;   // [m]   static sag the calibration targets
export const TyreReferenceSidewall    = 25000.0; // [N/m] the spring model's sidewall stiffness
export const TyreReferenceAir         = 0.9;     // [N/m/Pa] the spring model's air stiffness
export const TyreReferencePa          = 220000.0; // [Pa]

// 📐 Pressure stiffening shared by every model: the air column adds stiffness in proportion to the gauge pressure.
export function AirStiffness(Pressure) { return TyreReferenceAir * Math.max(0.0, Pressure); }

// -------------------------------------------------------------------------------------------------------------------------------
// Spring: F = (k_sidewall + k_air · p) · δ
// -------------------------------------------------------------------------------------------------------------------------------
export class SpringTyreModel {
    constructor() { this.Kind = TyreModelKinds.Spring; }
    Force(Compression, Pressure) {
        return (TyreReferenceSidewall + AirStiffness(Pressure)) * Math.max(0.0, Compression);
    }
}

// -------------------------------------------------------------------------------------------------------------------------------
// XPBD ring: position-based dynamics on a closed carcass of N particles.
// -------------------------------------------------------------------------------------------------------------------------------
const RingParticles   = 24;
const RingRadius      = 0.32;     // [m]  carcass centreline radius
const RingMass        = 1200.0;   // [kg] effective quasi-static mass: keeps the pressure load's per-step motion sub-millimetre
const RingWidth       = 0.205;    // [m]  tread width: the pressure load per unit length is p · width
const EdgeCompliance  = 1.0e-7;   // [m/N] stiff carcass membrane
const RelaxSteps      = 900;      // [-]  static relaxation steps
const RelaxDt         = 1.0 / 240.0;
const RelaxDamping    = 0.97;
const RingIterations  = 4;

// 📐 One static relaxation of the ring: pressure pushes the carcass out, spokes hold it on the rim, the road holds it up.
//    Returns the road reaction (N), taken from the ground projection's momentum over the last step.
function RelaxRing(Pressure, SpokeCompliance, HubHeight, Ground)
{
    // Returns { Reaction, X }: the road reaction (N) and the relaxed carcass positions.
    const N = RingParticles;
    const Hub = [0.0, HubHeight];
    const X = new Float64Array(2 * N), Prior = new Float64Array(2 * N), V = new Float64Array(2 * N);
    for (let I = 0; I < N; ++I)
    {
        const Theta = 2.0 * Math.PI * I / N;
        X[2 * I] = Prior[2 * I] = Hub[0] + RingRadius * Math.cos(Theta);
        X[2 * I + 1] = Prior[2 * I + 1] = Hub[1] + RingRadius * Math.sin(Theta);
    }
    const EdgeRest = 2.0 * RingRadius * Math.sin(Math.PI / N);
    const Mass = RingMass / N;
    const PerLength = Pressure * RingWidth * (2.0 * Math.PI * RingRadius / N);   // N per particle, outward
    const DtSq = RelaxDt * RelaxDt;
    const AlphaEdge = EdgeCompliance / DtSq;
    const AlphaSpoke = SpokeCompliance / DtSq;
    let GroundReaction = 0.0;
    for (let Step = 0; Step < RelaxSteps; ++Step)
    {
        // Predict: external pressure load, then integrate.
        for (let I = 0; I < N; ++I)
        {
            const Px = X[2 * I] - Hub[0], Py = X[2 * I + 1] - Hub[1];
            const Length = Math.hypot(Px, Py) || 1.0;
            V[2 * I]     += RelaxDt * PerLength * Px / Length / Mass;
            V[2 * I + 1] += RelaxDt * PerLength * Py / Length / Mass;
            X[2 * I]     += RelaxDt * V[2 * I];
            X[2 * I + 1] += RelaxDt * V[2 * I + 1];
        }
        GroundReaction = 0.0;
        for (let Iteration = 0; Iteration < RingIterations; ++Iteration)
        {
            // Carcass membrane between neighbours.
            for (let I = 0; I < N; ++I)
            {
                const J = (I + 1) % N;
                const Dx = X[2 * J] - X[2 * I], Dy = X[2 * J + 1] - X[2 * I + 1];
                const Length = Math.hypot(Dx, Dy) || 1e-9;
                const C = Length - EdgeRest;
                const Lambda = -C / (2.0 / Mass + AlphaEdge);
                const Nx = Dx / Length, Ny = Dy / Length;
                X[2 * I]     -= (Lambda / Mass) * Nx; X[2 * I + 1] -= (Lambda / Mass) * Ny;
                X[2 * J]     += (Lambda / Mass) * Nx; X[2 * J + 1] += (Lambda / Mass) * Ny;
            }
            // Spokes: each particle held at the carcass radius from the rim centre.
            for (let I = 0; I < N; ++I)
            {
                const Px = X[2 * I] - Hub[0], Py = X[2 * I + 1] - Hub[1];
                const Length = Math.hypot(Px, Py) || 1e-9;
                const C = Length - RingRadius;
                const Lambda = -C / (1.0 / Mass + AlphaSpoke);
                X[2 * I]     += (Lambda / Mass) * (Px / Length);
                X[2 * I + 1] += (Lambda / Mass) * (Py / Length);
            }
            // Road: the unilateral contact. Its projection is the reaction.
            if (Ground)
            {
                for (let I = 0; I < N; ++I)
                {
                    if (X[2 * I + 1] < 0.0)
                    {
                        const Push = -X[2 * I + 1];
                        X[2 * I + 1] = 0.0;
                        if (Iteration === RingIterations - 1)
                            GroundReaction += Mass * Push / DtSq;
                    }
                }
            }
        }
        for (let K = 0; K < 2 * N; ++K)
            V[K] = (X[K] - Prior[K]) / RelaxDt * RelaxDamping;
        Prior.set(X);
    }
    return { Reaction: GroundReaction, X };
}

// 📝 The free, pressurised rest of the ring (no road): its lowest particle sets the height at which the road first touches.
function RestingHubHeight(Pressure, SpokeCompliance)
{
    // Relax without a road. The free ring does not depend on the hub height, so its lowest carcass point sits a fixed
    // distance below the hub; the hub height that puts that point on the road is the resting height.
    const Free = 0.6;
    const { X } = RelaxRing(Pressure, SpokeCompliance, Free, false);
    let Lowest = Infinity;
    for (let I = 1; I < 2 * RingParticles; I += 2) Lowest = Math.min(Lowest, X[I]);
    return Free - Lowest;
}

// 📐 Solves the ring's road reaction for a list of compressions (metres) at one pressure and spoke compliance.
function XpbdCurve(Pressure, SpokeCompliance, Compressions)
{
    const Rest = RestingHubHeight(Pressure, SpokeCompliance);
    return Compressions.map((Compression) => RelaxRing(Pressure, SpokeCompliance, Rest - Compression, true).Reaction);
}

// -------------------------------------------------------------------------------------------------------------------------------
// 3D FEM: linear tetrahedra over a block of tyre section, condensed onto the rim face.
// -------------------------------------------------------------------------------------------------------------------------------
const FemHalfWidth   = 0.12;    // [m]
const FemHalfLength  = 0.10;    // [m]
const FemHeight      = 0.12;    // [m] rim face at the top, belt at the bottom
const FemCounts      = [7, 3, 3];   // [-] nodes along x, y, z
const FemPoisson     = 0.40;    // [-] below 0.45 so linear tetrahedra do not lock volumetrically
const FemPatchRadius = 0.32;    // [m] contact radius of the carcass: the patch half-width is sqrt(2 R δ)

// 📐 Gauss–Jordan style dense solve with partial pivoting; A is row-major n×n, returns x.
function SolveDense(A, b, n)
{
    for (let Column = 0; Column < n; ++Column)
    {
        let Pivot = Column, Largest = Math.abs(A[Column * n + Column]);
        for (let Row = Column + 1; Row < n; ++Row)
            if (Math.abs(A[Row * n + Column]) > Largest) { Largest = Math.abs(A[Row * n + Column]); Pivot = Row; }
        if (Pivot !== Column)
        {
            for (let K = 0; K < n; ++K) { const T = A[Column * n + K]; A[Column * n + K] = A[Pivot * n + K]; A[Pivot * n + K] = T; }
            const T = b[Column]; b[Column] = b[Pivot]; b[Pivot] = T;
        }
        const Diagonal = A[Column * n + Column] || 1e-30;
        for (let Row = Column + 1; Row < n; ++Row)
        {
            const Factor = A[Row * n + Column] / Diagonal;
            if (Factor === 0) continue;
            for (let K = Column; K < n; ++K) A[Row * n + K] -= Factor * A[Column * n + K];
            b[Row] -= Factor * b[Column];
        }
    }
    const X = new Float64Array(n);
    for (let Row = n - 1; Row >= 0; --Row)
    {
        let Sum = b[Row];
        for (let K = Row + 1; K < n; ++K) Sum -= A[Row * n + K] * X[K];
        X[Row] = Sum / (A[Row * n + Row] || 1e-30);
    }
    return X;
}

// 📐 The block: nodes on a regular grid, each hexahedron split into six tetrahedra along the same diagonal (Kuhn), so the
//    split conforms across neighbours. Returns nodes, tetrahedra and the per-node level (0 = belt, top = rim).
function BuildFemBlock()
{
    const [Nx, Ny, Nz] = FemCounts;
    const Nodes = [];
    const Index = (I, J, K) => (K * Ny + J) * Nx + I;
    for (let K = 0; K < Nz; ++K)
        for (let J = 0; J < Ny; ++J)
            for (let I = 0; I < Nx; ++I)
                Nodes.push([-FemHalfWidth + 2 * FemHalfWidth * I / (Nx - 1),
                            -FemHalfLength + 2 * FemHalfLength * J / (Ny - 1),
                            FemHeight * K / (Nz - 1)]);
    const Tets = [];
    const Corner = (I, J, K, C) => Index(I + (C & 1), J + ((C >> 1) & 1), K + ((C >> 2) & 1));
    const Kuhn = [[0, 1, 3, 7], [0, 1, 5, 7], [0, 2, 3, 7], [0, 2, 6, 7], [0, 4, 5, 7], [0, 4, 6, 7]];
    for (let K = 0; K < Nz - 1; ++K)
        for (let J = 0; J < Ny - 1; ++J)
            for (let I = 0; I < Nx - 1; ++I)
                for (const Split of Kuhn)
                    Tets.push(Split.map((C) => Corner(I, J, K, C)));
    return { Nodes, Tets, Index };
}

// 📐 Element stiffness of one linear tetrahedron for unit Young's modulus; returns the 12×12 matrix and the volume.
function TetStiffness(Points, Poisson)
{
    const [P0, P1, P2, P3] = Points;
    const Edge = (A, B) => [B[0] - A[0], B[1] - A[1], B[2] - A[2]];
    const M = [Edge(P0, P1), Edge(P0, P2), Edge(P0, P3)];   // columns of the Jacobian
    const Det = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])
              + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
    const Volume = Math.abs(Det) / 6.0;
    if (Volume < 1e-14) return { K: new Float64Array(144), Volume: 0 };
    // Inverse Jacobian rows give the shape-function gradients.
    const Inv = [
        [(M[1][1] * M[2][2] - M[1][2] * M[2][1]) / Det, (M[0][2] * M[2][1] - M[0][1] * M[2][2]) / Det, (M[0][1] * M[1][2] - M[0][2] * M[1][1]) / Det],
        [(M[1][2] * M[2][0] - M[1][0] * M[2][2]) / Det, (M[0][0] * M[2][2] - M[0][2] * M[2][0]) / Det, (M[0][2] * M[1][0] - M[0][0] * M[1][2]) / Det],
        [(M[1][0] * M[2][1] - M[1][1] * M[2][0]) / Det, (M[0][1] * M[2][0] - M[0][0] * M[2][1]) / Det, (M[0][0] * M[1][1] - M[0][1] * M[1][0]) / Det]
    ];
    const G = [[0, 0, 0]];
    for (let A = 0; A < 3; ++A) G.push([Inv[0][A], Inv[1][A], Inv[2][A]]);   // ∇N_a = column a of inv(code M)
    const Grad = [[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]];
    for (let A = 0; A < 3; ++A)
        for (let D = 0; D < 3; ++D) { Grad[A + 1][D] = G[A + 1][D]; Grad[0][D] -= G[A + 1][D]; }
    // Isotropic elasticity, unit E: D = E/((1+ν)(1−2ν)) · [[1−ν, ν, ν, 0, 0, 0], …]
    const Lam = Poisson / ((1 + Poisson) * (1 - 2 * Poisson));
    const Mu  = 1.0 / (2 * (1 + Poisson));
    const D = [[Lam + 2 * Mu, Lam, Lam, 0, 0, 0], [Lam, Lam + 2 * Mu, Lam, 0, 0, 0], [Lam, Lam, Lam + 2 * Mu, 0, 0, 0],
               [0, 0, 0, Mu, 0, 0], [0, 0, 0, 0, Mu, 0], [0, 0, 0, 0, 0, Mu]];
    // B: 6×12 strain-displacement.
    const B = Array.from({ length: 6 }, () => new Float64Array(12));
    for (let A = 0; A < 4; ++A)
    {
        const [Gx, Gy, Gz] = Grad[A];
        const C = 3 * A;
        B[0][C] = Gx;
        B[1][C + 1] = Gy;
        B[2][C + 2] = Gz;
        B[3][C] = Gy; B[3][C + 1] = Gx;
        B[4][C + 1] = Gz; B[4][C + 2] = Gy;
        B[5][C] = Gz; B[5][C + 2] = Gx;
    }
    const K = new Float64Array(144);
    for (let I = 0; I < 12; ++I)
        for (let J = 0; J < 12; ++J)
        {
            let Sum = 0.0;
            for (let R = 0; R < 6; ++R)
                for (let S = 0; S < 6; ++S)
                    Sum += B[R][I] * D[R][S] * B[S][J];
            K[I * 12 + J] = Sum * Volume;
        }
    return { K, Volume };
}

// 📝 The block's global stiffness for unit modulus, assembled once and reused for every compression.
let FemCache = null;
function AssembleFem()
{
    if (FemCache) return FemCache;
    const { Nodes, Tets } = BuildFemBlock();
    const Dof = 3 * Nodes.length;
    const K = new Float64Array(Dof * Dof);
    for (const Tet of Tets)
    {
        const { K: Ke } = TetStiffness(Tet.map((Node) => Nodes[Node]), FemPoisson);
        for (let I = 0; I < 4; ++I)
            for (let A = 0; A < 3; ++A)
                for (let J = 0; J < 4; ++J)
                    for (let B = 0; B < 3; ++B)
                        K[(3 * Tet[I] + A) * Dof + 3 * Tet[J] + B] += Ke[(3 * I + A) * 12 + 3 * J + B];
    }
    FemCache = { Nodes, Dof, K };
    return FemCache;
}

// 📐 Reaction of the block (unit modulus) at one compression: the rim face moves down, the belt nodes inside the contact patch
//    are pinned, the rest is condensed out by solving the free block.
function FemReaction(Compression)
{
    const { Nodes, Dof, K } = AssembleFem();
    const Radius = Math.sqrt(2.0 * FemPatchRadius * Math.max(0.0, Compression));
    const Prescribed = new Float64Array(Dof);
    const Fixed = new Uint8Array(Dof);
    const Rim = Nodes.map((_, I) => I).filter((I) => Math.abs(Nodes[I][2] - FemHeight) < 1e-9);
    for (const I of Rim)
    {
        Fixed[3 * I] = Fixed[3 * I + 1] = Fixed[3 * I + 2] = 1;
        Prescribed[3 * I + 2] = -Compression;
    }
    const Belt = Nodes.map((_, I) => I).filter((I) => Math.abs(Nodes[I][2]) < 1e-9 && Math.abs(Nodes[I][0]) <= Radius + 1e-9);
    for (const I of Belt)
    {
        Fixed[3 * I] = Fixed[3 * I + 1] = Fixed[3 * I + 2] = 1;
    }
    const Free = [];
    for (let D = 0; D < Dof; ++D) if (!Fixed[D]) Free.push(D);
    const n = Free.length;
    const A = new Float64Array(n * n), b = new Float64Array(n);
    for (let R = 0; R < n; ++R)
    {
        const Row = Free[R];
        let Rhs = 0.0;
        for (let C = 0; C < Dof; ++C)
            if (Fixed[C]) Rhs -= K[Row * Dof + C] * Prescribed[C];
        b[R] = Rhs;
        for (let C = 0; C < n; ++C) A[R * n + C] = K[Row * Dof + Free[C]];
    }
    const Solution = SolveDense(A, b, n);
    const U = new Float64Array(Dof);
    for (let D = 0; D < Dof; ++D) U[D] = Fixed[D] ? Prescribed[D] : 0.0;
    for (let C = 0; C < n; ++C) U[Free[C]] = Solution[C];
    // Reaction: the vertical internal force carried by the pinned belt nodes.
    let Reaction = 0.0;
    for (const I of Belt)
    {
        let Force = 0.0;
        for (let C = 0; C < Dof; ++C) Force += K[(3 * I + 2) * Dof + C] * U[C];
        Reaction += Force;
    }
    return Math.abs(Reaction);
}

// -------------------------------------------------------------------------------------------------------------------------------
// Tabulated, calibrated models
// -------------------------------------------------------------------------------------------------------------------------------
const CurveSamples = 24;
const CurveMaximum = 0.05;   // [m]

// 📝 Each nonlinear model holds one table per pressure bucket: radial force against compression, linear between samples.
class TabulatedTyreModel {
    constructor(Kind, Build) {
        this.Kind = Kind;
        this.Build = Build;
        this.Tables = new Map();
    }
    Table(Pressure) {
        const Bucket = Math.round(Math.max(0, Pressure) / 5000.0) * 5000.0;
        if (!this.Tables.has(Bucket)) {
            if (this.Tables.size > 16) this.Tables.clear();
            this.Tables.set(Bucket, this.Build(Bucket));
        }
        return this.Tables.get(Bucket);
    }
    Force(Compression, Pressure) {
        const Table = this.Table(Pressure);
        const Clamped = Math.min(CurveMaximum, Math.max(0.0, Compression));
        const T = Clamped / CurveMaximum * (CurveSamples - 1);
        const Low = Math.min(CurveSamples - 2, Math.floor(T));
        const Fraction = T - Low;
        return Table[Low] * (1.0 - Fraction) + Table[Low + 1] * Fraction;
    }
}

// 📝 XPBD: the spoke compliance is calibrated (a secant search on the static reaction at the reference compression) so the
//    ring reproduces the spring model's sag; the curve then carries the patch growth.
let XpbdCalibration = null;
function CalibrateXpbd(Pressure) {
    const Target = (TyreReferenceSidewall + AirStiffness(Pressure)) * TyreReferenceCompression;
    const Compressions = Array.from({ length: CurveSamples }, (_, I) => CurveMaximum * I / (CurveSamples - 1));
    const Probe = (Compliance) => XpbdCurve(Pressure, Compliance, [TyreReferenceCompression])[0];
    let Low = 1.0e-6, High = 1.0e-3;
    let FLow = Probe(Low) - Target, FHigh = Probe(High) - Target;
    for (let Iteration = 0; Iteration < 8 && FLow * FHigh > 0; ++Iteration) {
        Low /= 4; High *= 4;
        FLow = Probe(Low) - Target; FHigh = Probe(High) - Target;
    }
    let Compliance = Low;
    for (let Iteration = 0; Iteration < 10; ++Iteration) {
        Compliance = 0.5 * (Low + High);
        const F = Probe(Compliance) - Target;
        if (Math.abs(F) < 0.01 * Target) break;
        if (F * FLow > 0) { Low = Compliance; FLow = F; } else { High = Compliance; FHigh = F; }
    }
    return { Compliance, Compressions, Curve: XpbdCurve(Pressure, Compliance, Compressions) };
}

// 📝 FEM: one modulus, calibrated so the block reproduces the spring's sag at the reference compression.
let FemModulus = null;
function CalibrateFem() {
    const Target = (TyreReferenceSidewall) * TyreReferenceCompression;
    const Unit = FemReaction(TyreReferenceCompression);
    return Target / Unit;
}

// 📝 Built at bring-up (not per frame): the reference-pressure table is computed on creation, so the first contact frame
//    never stalls. Other pressure buckets are built on first use and cached.
export function CreateTyreModel(Kind)
{
    let Model;
    if (Kind === TyreModelKinds.Xpbd) {
        Model = new TabulatedTyreModel(TyreModelKinds.Xpbd, (Pressure) => {
            const { Curve } = CalibrateXpbd(Pressure);
            XpbdCalibration = { Pressure, Curve };
            return Curve.map((Force) => Math.max(0.0, Force));
        });
    } else if (Kind === TyreModelKinds.Fem) {
        if (FemModulus === null) FemModulus = CalibrateFem();
        Model = new TabulatedTyreModel(TyreModelKinds.Fem, (Pressure) => {
            const Compressions = Array.from({ length: CurveSamples }, (_, I) => CurveMaximum * I / (CurveSamples - 1));
            return Compressions.map((Compression) => FemModulus * FemReaction(Compression) + AirStiffness(Pressure) * Compression);
        });
    } else {
        return new SpringTyreModel();
    }
    Model.Table(TyreReferencePa);
    return Model;
}

// 📝 Test hooks: the calibration values the models settled on.
export const TyreModelDiagnostics = {
    XpbdCalibration: () => XpbdCalibration,
    FemModulus: () => FemModulus,
    FemOperator: () => AssembleFem(),
    FemReaction: (Compression) => FemReaction(Compression)
};
