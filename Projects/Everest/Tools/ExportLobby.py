#!/usr/bin/env python3
"""Convert a reference Lobby capture to this native renderer's archive format.

The FSP1 writer comes from upstream RestirGiParity; the current JS material
record has an extra refraction index the older native shader does not support.
"""
import pathlib
import shutil
import struct
import sys


def read_writes(path):
    data = path.read_bytes()
    pos = 0
    while pos < len(data):
        refresh, category, n = struct.unpack_from('<iBH', data, pos)
        pos += 7
        label = data[pos:pos+n].decode('utf-8')
        pos += n
        offset = struct.unpack_from('<Q', data, pos)[0]
        pos += 24  # byte offset plus the capture's 16-byte extent
        count = struct.unpack_from('<I', data, pos)[0]
        pos += 4
        payload = data[pos:pos+count]
        if len(payload) != count:
            raise ValueError('truncated write log')
        pos += count
        yield refresh, category, label, offset, payload


def main():
    if len(sys.argv) != 4:
        raise SystemExit('usage: ExportLobby.py CAPTURE OUTPUT PROVING_GROUND_ARCHIVE')
    capture, output, proving = map(pathlib.Path, sys.argv[1:])
    output.mkdir(parents=True, exist_ok=True)
    writes = list(read_writes(capture / 'WriteLog.bin'))
    material = None
    for refresh, category, label, offset, data in writes:
        if refresh == -1 and label == 'Materials':
            if category == 1:
                material = bytearray(data)
            elif category == 0 and material is not None:
                if offset + len(data) > len(material):
                    raise ValueError('material rewrite exceeds table')
                material[offset:offset+len(data)] = data
    if material is None or len(material) % 64:
        raise ValueError('expected 64-byte reference material records')
    (output / 'Materials.bin').write_bytes(b''.join(material[i:i+48] for i in range(0, len(material), 64)))
    for label, name, size in [('Vehicle record', 'VehicleProfile.bin', 10448), ('Effect record', 'EffectProfile.bin', 1120)]:
        choices = [data for refresh, _, written, offset, data in writes if refresh == 0 and written == label and offset == 0]
        if not choices or len(choices[-1]) > size:
            raise ValueError(f'invalid or missing {label}')
        (output / name).write_bytes(choices[-1].ljust(size, b'\0'))
    shutil.copyfile(capture / 'Scene.facetspace', output / 'Lobby.facetspace')
    # The Lobby disables clouds, and the current reference does not create these
    # older cloud buffers. Reuse the native Proving Ground's disabled profiles.
    for name in ['CloudProfile.bin', 'CloudShadowProfile.bin']:
        shutil.copyfile(proving / name, output / name)


if __name__ == '__main__':
    main()
