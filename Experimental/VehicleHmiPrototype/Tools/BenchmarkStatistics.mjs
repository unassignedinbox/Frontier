//============================================================================================================================================
//                                                        BENCHMARKSTATISTICS.MJS
//============================================================================================================================================
// 🧩 The arithmetic of the toggle benchmarks, kept free of I/O so it is unit-tested (BenchmarkTools.test.mjs): robust statistics for one run, the comparison of a
//    variant with its baseline, and the verdict.
//
// 📝 How a verdict is reached. A job runs the baseline and the variant interleaved (A B A B ...) on ONE runner, so machine differences between runners cancel. The spread
//    of the baseline's own repeats is the noise floor: a variant is only called faster or slower when its ratio to the baseline leaves BOTH a fixed 5 % band and twice that
//    noise. Anything inside is "no measurable difference": the honest answer for a toggle whose cost is below what a shared software-rendering runner can resolve.

export const MinimumEffect = 0.05;

export function Median(Values)
{
    if (!Values.length) return NaN;
    const Sorted = [...Values].sort((A, B) => A - B), Middle = Sorted.length >> 1;
    return Sorted.length % 2 ? Sorted[Middle] : 0.5 * (Sorted[Middle - 1] + Sorted[Middle]);
}

export function Summarise(Values)
{
    if (!Values.length) return { Count: 0, MedianMs: NaN, MeanMs: NaN, MinMs: NaN, MaxMs: NaN, SpreadFraction: NaN };
    const Median_ = Median(Values), Mean = Values.reduce((Sum, Value) => Sum + Value, 0) / Values.length;
    // median absolute deviation as a fraction of the median: how much the frames of ONE run disagree with each other
    const Spread = Median(Values.map((Value) => Math.abs(Value - Median_))) / Median_;
    return { Count: Values.length, MedianMs: Median_, MeanMs: Mean, MinMs: Math.min(...Values), MaxMs: Math.max(...Values), SpreadFraction: Spread };
}

// ➡️ Baseline and variant runs (arrays of per-run medians in ms, in the order they ran) → ratio, noise, verdict.
export function Compare(BaselineRuns, VariantRuns)
{
    const Baseline = Median(BaselineRuns), Variant = Median(VariantRuns);
    const Ratio = Variant / Baseline;
    // noise: the largest relative disagreement between repeats of the same configuration
    const Relative = (Runs) => Runs.length < 2 ? 0 : (Math.max(...Runs) - Math.min(...Runs)) / Median(Runs);
    const Noise = Math.max(Relative(BaselineRuns), Relative(VariantRuns));
    const Band = Math.max(MinimumEffect, 2 * Noise);
    const Verdict = !Number.isFinite(Ratio) ? "no data" : Ratio < 1 - Band ? "faster" : Ratio > 1 + Band ? "slower" : "no measurable difference";
    return { BaselineMs: Baseline, VariantMs: Variant, Ratio, Noise, Band, Verdict };
}

export const Percent = (Ratio) => `${Ratio >= 1 ? "+" : "−"}${(Math.abs(Ratio - 1) * 100).toFixed(1)} %`;

// ➡️ Markdown for the job summary: one row per toggle.
export function RenderTable(Results, Machine = {})
{
    const Lines = [];
    Lines.push(`**Machine**: ${Machine.Cpu ?? "unknown CPU"} × ${Machine.Cores ?? "?"} cores, Node ${Machine.Node ?? "?"}, SwiftShader ${Machine.SwiftShader ?? "?"}.`);
    Lines.push("");
    Lines.push("> 🔴 GitHub runners have **no GPU**. Every figure below is the shader code executing on CPU cores (SwiftShader): a measure of *relative* ALU and branch work between two");
    Lines.push("> configurations, not of GPU time. It says nothing about memory bandwidth, occupancy or fixed-function hardware, so a ranking here is a hypothesis for a real GPU, not a result.");
    Lines.push("");
    Lines.push("| Toggle | Baseline | Variant | Baseline ms/frame | Variant ms/frame | Variant vs baseline | Noise | Verdict |");
    Lines.push("|---|---|---|---:|---:|---:|---:|---|");
    for (const Result of Results)
    {
        const Comparison = Result.Comparison;
        Lines.push(`| ${Result.Label} | \`${Result.Baseline}\` | \`${Result.Variant}\` | ${Comparison.BaselineMs.toFixed(0)} | ${Comparison.VariantMs.toFixed(0)} | ${Percent(Comparison.Ratio)} | ±${(Comparison.Noise * 100).toFixed(1)} % | **${Comparison.Verdict}** |`);
    }
    return Lines.join("\n");
}
