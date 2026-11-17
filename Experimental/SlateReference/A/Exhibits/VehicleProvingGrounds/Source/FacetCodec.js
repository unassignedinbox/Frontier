//============================================================================================================================================
//                                                              FACETCODEC.JS
//============================================================================================================================================
// 🧩 Decodes the 'FCT1' facet stream written by Archive/FacetArchiveCodec.py into typed position, facet and family spans.

const StreamMagic          = "FCT1";
const FixedRecordExtent         = 48;   // [B] - fixed header ahead of the payload
const CornerNormalsPresent = 1;    // [-] - flag bit: nine normal scalars per facet follow the families
const FacetCache           = new Map();

export async function RetrieveFacetStream(Location)
{
    if (FacetCache.has(Location))
        return FacetCache.get(Location);
    let LastError = null;
    for (let Attempt = 0; Attempt < 4; ++Attempt)
    {
        try
        {
            const Response = await fetch(Location, { cache: "no-cache" });
            if (Response.ok)
            {
                const Decoded = DecodeFacetStream(await Response.arrayBuffer(), Location);
                FacetCache.set(Location, Decoded);
                return Decoded;
            }
            LastError = new Error(`Facet stream ${Location} could not be retrieved (${Response.status})`);
            if (Response.status < 500 && Response.status !== 429)
                break;
        }
        catch (Err) { LastError = Err; }
        await new Promise((Resolve) => setTimeout(Resolve, 200 * (Attempt + 1)));
    }
    throw LastError || new Error(`Facet stream ${Location} could not be retrieved`);
}

export function DecodeFacetStream(Bytes, Location = "")
{
    const View = new DataView(Bytes);
    const Magic = String.fromCharCode(View.getUint8(0), View.getUint8(1), View.getUint8(2), View.getUint8(3));
    if (Magic !== StreamMagic)
        throw new Error(`${Location}: not a facet stream (magic '${Magic}')`);

    const VertexCount = View.getUint32(4, true);
    const FacetCount  = View.getUint32(8, true);
    const Flags       = View.getUint32(12, true);
    const BoundsMinimum = [View.getFloat32(16, true), View.getFloat32(20, true), View.getFloat32(24, true)];
    const BoundsMaximum = [View.getFloat32(28, true), View.getFloat32(32, true), View.getFloat32(36, true)];

    let Cursor = FixedRecordExtent;
    const Positions = new Float32Array(Bytes, Cursor, VertexCount * 3);
    Cursor += VertexCount * 12;
    const Facets = new Uint32Array(Bytes, Cursor, FacetCount * 3);
    Cursor += FacetCount * 12;
    const Families = new Uint32Array(Bytes, Cursor, FacetCount);
    Cursor += FacetCount * 4;
    let CornerNormals = null;
    if (Flags & CornerNormalsPresent)
    {
        CornerNormals = new Float32Array(Bytes, Cursor, FacetCount * 9);
        Cursor += FacetCount * 36;
    }
    if (Cursor !== Bytes.byteLength)
        throw new Error(`${Location}: stream length ${Bytes.byteLength} disagrees with its header (${Cursor})`);

    return { VertexCount, FacetCount, Positions, Facets, Families, CornerNormals, BoundsMinimum, BoundsMaximum };
}
