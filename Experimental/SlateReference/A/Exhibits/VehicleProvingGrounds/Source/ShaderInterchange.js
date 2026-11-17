//============================================================================================================================================
//                                                           SHADERINTERCHANGE.JS
//============================================================================================================================================
// 🧩 WGSL source retrieval, include expansion and constant injection, with diagnostics mapped back to the source file and line.

const IncludePattern = /^\s*\/\/!\s*include\s+"([^"]+)"\s*$/;

export class ShaderInterchange
{
    constructor(Exchange, SourceFolder)
    {
        this.Exchange     = Exchange;
        this.SourceFolder = SourceFolder;
        this.Sources      = new Map();
        this.Modules      = new Map();
    }

    async RetrieveSources(FileNames)
    {
        // ① Fast single-request shader bundle (1 HTTP request instead of 55+ simultaneous sockets through proxies!)
        if (this.SourceFolder === "Shader")
        {
            for (let Attempt = 0; Attempt < 3; ++Attempt)
            {
                try
                {
                    const BundleRes = await fetch("api/shaders.json", { cache: "no-cache" });
                    if (BundleRes.ok)
                    {
                        const Bundle = await BundleRes.json();
                        let Complete = true;
                        for (const FileName of FileNames)
                        {
                            if (typeof Bundle[FileName] === "string")
                                this.Sources.set(FileName, Bundle[FileName]);
                            else
                                Complete = false;
                        }
                        if (Complete)
                            return;
                        break;
                    }
                    if (BundleRes.status === 404)
                        break; // Static host without bundle endpoint — fall back to batched individual fetches
                }
                catch { /* transient network/proxy hiccup — retry */ }
                await new Promise((Resolve) => setTimeout(Resolve, 180 * (Attempt + 1)));
            }
        }

        // ② Fallback: bounded-concurrency fetches (max 6 in flight) with exponential retry on transient 5xx/network errors
        const Pending = FileNames.filter((Name) => !this.Sources.has(Name));
        const Concurrency = 6;
        let Cursor = 0;
        const Worker = async () =>
        {
            while (Cursor < Pending.length)
            {
                const FileName = Pending[Cursor++];
                let LastError = null;
                for (let Attempt = 0; Attempt < 4; ++Attempt)
                {
                    try
                    {
                        const Response = await fetch(`${this.SourceFolder}/${FileName}`, { cache: "no-cache" });
                        if (Response.ok)
                        {
                            this.Sources.set(FileName, await Response.text());
                            LastError = null;
                            break;
                        }
                        LastError = new Error(`Shader source ${FileName} could not be retrieved (${Response.status})`);
                        if (Response.status < 500 && Response.status !== 429)
                            break;
                    }
                    catch (Err) { LastError = Err; }
                    await new Promise((Resolve) => setTimeout(Resolve, 200 * (Attempt + 1)));
                }
                if (LastError)
                    throw LastError;
            }
        };
        await Promise.all(Array.from({ length: Math.min(Concurrency, Pending.length) }, () => Worker()));
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                               SOURCE ASSEMBLY
    //--------------------------------------------------------------------------------------------------------------------

    // 📝 Returns the expanded text plus one origin entry per emitted line, so a diagnostic names the authored file.
    AssembleSource(FileName, Constants)
    {
        const Lines   = [];
        const Origins = [];
        for (const [Name, Specification] of Object.entries(Constants))
        {
            Lines.push(`const ${Name}: ${Specification.Type} = ${ProjectConstantLiteral(Specification)};`);
            Origins.push({ File: "(constants)", Line: Lines.length });
        }
        const Visited = new Set();
        const Expand = (Current) =>
        {
            if (Visited.has(Current))
                return;
            Visited.add(Current);
            const Text = this.Sources.get(Current);
            if (Text === undefined)
                throw new Error(`Shader include ${Current} was never retrieved`);
            const SourceLines = Text.split("\n");
            for (let LineIndex = 0; LineIndex < SourceLines.length; ++LineIndex)
            {
                const Match = SourceLines[LineIndex].match(IncludePattern);
                if (Match)
                {
                    Expand(Match[1]);
                    continue;
                }
                Lines.push(SourceLines[LineIndex]);
                Origins.push({ File: Current, Line: LineIndex + 1 });
            }
        };
        Expand(FileName);
        return { Text: Lines.join("\n"), Origins };
    }

    async ConstructModule(FileName, Constants = {}, Label = FileName)
    {
        const Key = `${FileName}|${JSON.stringify(Constants)}`;
        if (this.Modules.has(Key))
            return this.Modules.get(Key);
        const Assembly = this.AssembleSource(FileName, Constants);
        const Module = this.Exchange.Device.createShaderModule({ label: Label, code: Assembly.Text });
        const Information = await Module.getCompilationInfo();
        const Errors = Information.messages.filter((Message) => Message.type === "error");
        for (const Message of Information.messages)
        {
            const Origin = Assembly.Origins[Math.max(0, Message.lineNum - 1)] || { File: FileName, Line: Message.lineNum };
            const Report = `${Origin.File}:${Origin.Line}:${Message.linePos} ${Message.type}: ${Message.message}`;
            if (Message.type === "error")
                console.error(`[WGSL] ${Report}`);
            else
                console.warn(`[WGSL] ${Report}`);
        }
        if (Errors.length > 0)
        {
            const First = Errors[0];
            const Origin = Assembly.Origins[Math.max(0, First.lineNum - 1)] || { File: FileName, Line: First.lineNum };
            throw new Error(`WGSL compilation failed in ${Origin.File}:${Origin.Line} — ${First.message}`);
        }
        this.Modules.set(Key, Module);
        return Module;
    }

    //--------------------------------------------------------------------------------------------------------------------
    //                                             PROGRAM CONSTRUCTION
    //--------------------------------------------------------------------------------------------------------------------

    async ConstructComputeProgram(FileName, EntryPoint, Layouts, Constants = {})
    {
        const Module = await this.ConstructModule(FileName, Constants);
        const Device = this.Exchange.Device;
        Device.pushErrorScope("validation");
        const Program = await Device.createComputePipelineAsync({
            label:   `${FileName}:${EntryPoint}`,
            layout:  Device.createPipelineLayout({ bindGroupLayouts: Layouts }),
            compute: { module: Module, entryPoint: EntryPoint }
        });
        const Failure = await Device.popErrorScope();
        if (Failure)
            throw new Error(`Compute program ${FileName}:${EntryPoint} — ${Failure.message}`);
        return Program;
    }

    async ConstructRasterProgram(FileName, Descriptor, Layouts, Constants = {})
    {
        const Module = await this.ConstructModule(FileName, Constants);
        const Device = this.Exchange.Device;
        const Complete = {
            label:  `${FileName}:${Descriptor.VertexEntry}`,
            layout: Device.createPipelineLayout({ bindGroupLayouts: Layouts }),
            vertex: { module: Module, entryPoint: Descriptor.VertexEntry, buffers: Descriptor.VertexLayouts || [] },
            primitive: Descriptor.Primitive || { topology: "triangle-list", cullMode: "none" },
            depthStencil: Descriptor.DepthStencil,
            multisample: { count: 1 }
        };
        if (Descriptor.FragmentEntry)
            Complete.fragment = { module: Module, entryPoint: Descriptor.FragmentEntry, targets: Descriptor.Targets };
        Device.pushErrorScope("validation");
        const Program = await Device.createRenderPipelineAsync(Complete);
        const Failure = await Device.popErrorScope();
        if (Failure)
            throw new Error(`Raster program ${FileName}:${Descriptor.VertexEntry} — ${Failure.message}`);
        return Program;
    }
}

function ProjectConstantLiteral(Specification)
{
    const Scalar = Specification.Value;
    switch (Specification.Type)
    {
        case "u32":  return `${Math.max(0, Math.round(Scalar))}u`;
        case "i32":  return `${Math.round(Scalar)}i`;
        case "f32":  return Number.isInteger(Scalar) ? `${Scalar}.0` : `${Scalar}`;
        case "bool": return Scalar ? "true" : "false";
        default:     return `${Scalar}`;
    }
}
