//============================================================================================================================================
//                                                        BENCHMARKTOOLS.TEST.MJS
//============================================================================================================================================
// 🧩 The benchmark tooling is tested like everything else: its statistics, its verdict (it must say "no measurable difference" when that is the truth), the run order, the
//    summary table, and (on Dawn's null backend, where nothing executes) the whole plumbing from the harness through BenchHooks.mjs to a result file.

import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { Median, Summarise, Compare, RenderTable, Percent, MinimumEffect } from "./BenchmarkStatistics.mjs";
import { ParseArguments, RunOrder } from "./RunBenchmark.mjs";
import { Summarise as SummariseDirectory } from "./SummariseBenchmarks.mjs";

const Here = path.dirname(fileURLToPath(import.meta.url));

test("median and the robust spread", () =>
{
    assert.equal(Median([3, 1, 2]), 2); assert.equal(Median([4, 1, 3, 2]), 2.5); assert.ok(Number.isNaN(Median([])));
    const Statistics = Summarise([100, 102, 98, 101, 99, 5000]);       // one slow frame (a GC pause, a noisy neighbour) must not move the median
    assert.ok(Math.abs(Statistics.MedianMs - 100.5) < 1e-9 && Statistics.MaxMs === 5000 && Statistics.MeanMs > 800, "the median ignores the outlier, the mean does not");
    assert.ok(Statistics.SpreadFraction < 0.02, `the spread is the median absolute deviation (${Statistics.SpreadFraction})`);
    assert.equal(Summarise([]).Count, 0);
});

test("verdict: faster, slower, and — when the truth is that — no measurable difference", () =>
{
    assert.equal(Compare([100, 101], [80, 81]).Verdict, "faster");
    assert.equal(Compare([100, 101], [125, 124]).Verdict, "slower");
    assert.equal(Compare([100, 101], [101, 100]).Verdict, "no measurable difference", "an identical configuration is not faster or slower");
    assert.equal(Compare([100, 100], [103, 104]).Verdict, "no measurable difference", "a 3-4 % difference is inside the 5 % band");
    // a noisy machine widens the band: 20 % spread between repeats of the BASELINE means a 25 % gap is not believed
    const Noisy = Compare([100, 120], [85, 86]);
    assert.ok(Noisy.Noise > 0.15 && Noisy.Band >= 2 * Noisy.Noise - 1e-12 && Noisy.Verdict === "no measurable difference", `noise ${Noisy.Noise}, band ${Noisy.Band}, ${Noisy.Verdict}`);
    assert.equal(Compare([100], [70]).Verdict, "faster", "a single repeat has no noise estimate: the fixed band alone decides");
    assert.equal(Compare([], [1]).Verdict, "no data");
    assert.equal(MinimumEffect, 0.05);
    assert.equal(Percent(1.1234), "+12.3 %"); assert.equal(Percent(0.8), "−20.0 %");
});

test("the run order alternates and mirrors, so a drift cannot favour one side", () =>
{
    assert.deepEqual(RunOrder(2), ["baseline", "variant", "variant", "baseline"]);
    assert.deepEqual(RunOrder(3), ["baseline", "variant", "variant", "baseline", "baseline", "variant"]);
    for (const Rounds of [1, 2, 4]) { const Order = RunOrder(Rounds); assert.equal(Order.filter((Name) => Name === "baseline").length, Rounds); assert.equal(Order.filter((Name) => Name === "variant").length, Rounds); }
});

test("argument parsing rejects what would produce a meaningless benchmark", () =>
{
    const Good = ParseArguments(["--baseline", "renderer=bvh", "--variant", "renderer=sdf", "--frames", "12"]);
    assert.equal(Good.frames, 12); assert.equal(Good.rounds, 2); assert.equal(Good.label, "renderer=sdf"); assert.equal(Good.warmup, 3);
    assert.throws(() => ParseArguments(["--baseline", "a"]), /required/);
    assert.throws(() => ParseArguments(["--baseline", "a", "--variant", "b", "--frobnicate", "1"]), /unknown/);
    assert.throws(() => ParseArguments(["--baseline", "a", "--variant", "b", "--frames", "4"]), /exceed the warm-up/);
});

test("the summary table carries the honesty note and one row per toggle", () =>
{
    const Result = (Label, A, B) => ({ Label, Baseline: "renderer=bvh", Variant: `renderer=bvh&${Label}`, Machine: { Cpu: "Test CPU", Cores: 4, Node: "v22", SwiftShader: "pkg" }, Comparison: Compare([A, A + 1], [B, B + 1]) });
    const Table = RenderTable([Result("screentraces=1", 1000, 800), Result("lightgrid=0", 1000, 1001)], { Cpu: "Test CPU", Cores: 4, Node: "v22", SwiftShader: "pkg" });
    assert.match(Table, /no GPU/); assert.match(Table, /relative/); assert.match(Table, /Test CPU × 4 cores/);
    assert.match(Table, /\| screentraces=1 \|.*\*\*faster\*\*/); assert.match(Table, /\| lightgrid=0 \|.*\*\*no measurable difference\*\*/);
    assert.equal(Table.split("\n").filter((Line) => /^\| (screentraces|lightgrid)/.test(Line)).length, 2);
    const Directory = fs.mkdtempSync(path.join(os.tmpdir(), "bench-"));
    fs.writeFileSync(path.join(Directory, "bench-a.json"), JSON.stringify(Result("screentraces=1", 1000, 800)));
    fs.writeFileSync(path.join(Directory, "ignored.json"), JSON.stringify({ not: "a result" }));
    assert.match(SummariseDirectory([Directory]), /screentraces=1/);
    assert.match(SummariseDirectory([path.join(Directory, "nothing")]), /No benchmark results/);
});

test("plumbing: harness + BenchHooks + RunBenchmark produce a result file on the null backend", { timeout: 300000 }, () =>
{
    const Out = path.join(fs.mkdtempSync(path.join(os.tmpdir(), "bench-")), "bench-plumbing.json");
    const Run = spawnSync(process.execPath, [path.join(Here, "RunBenchmark.mjs"), "--label", "plumbing", "--baseline", "renderer=bvh", "--variant", "renderer=sdf", "--frames", "6", "--rounds", "1", "--warmup", "2", "--out", Out],
                          { cwd: Here, encoding: "utf8", env: { ...process.env, BACKEND: "null", SIZE: "320x180" }, timeout: 280000 });
    assert.equal(Run.status, 0, (Run.stdout + Run.stderr).slice(-1500));
    const Result = JSON.parse(fs.readFileSync(Out, "utf8"));
    assert.equal(Result.Runs.length, 2); assert.deepEqual(Result.Runs.map((Entry) => Entry.Which), ["baseline", "variant"]);
    for (const Entry of Result.Runs) { assert.ok(Entry.MedianMs >= 0 && Entry.DurationsMs.length === 6, "every executed frame was timed"); }
    assert.ok(["faster", "slower", "no measurable difference", "no data"].includes(Result.Comparison.Verdict));
    assert.match(Result.Machine.SwiftShader, /sparticuz/);
});
