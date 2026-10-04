"""Generate parser fixtures from the public schema; never read a player's saves."""
from pathlib import Path
import struct
import sys

def u32(value):
    return struct.pack('<I', value)

def string(value):
    encoded = value.encode('ascii') + b'\0'
    return u32(len(encoded)) + encoded

def save(class_name, fields):
    header = b'GVAS' + b''.join(map(u32, (3, 522, 1017)))
    header += struct.pack('<HHHI', 5, 6, 1, 0) + string('UE5')
    header += u32(3) + u32(93) + bytes(93 * 20)
    header += string('/Game/Mods/MCD2Graphics/' + class_name + '.' + class_name + '_C') + b'\0'
    for name, value in fields.items():
        header += string(name) + string('IntProperty') + u32(0) + u32(4) + b'\0' + u32(value)
    return header + string('None') + u32(0)

def main():
    output = Path(sys.argv[1])
    output.mkdir(parents=True, exist_ok=True)
    (output / 'settings.sav').write_bytes(save('GraphicsSettingsSave', {
        'SchemaVersion': 4, 'Revision': 22, 'ReconstructionMode': 2,
        'RenderScaleBasisPoints': 6667, 'SRPreset': 0, 'CustomScaleBasisPoints': 6700,
        'AppliedSourceRevision': 21, 'AppliedSourceSessionId': 345,
        'RenderContextReady': 1, 'RenderContextSessionId': 345,
    }))
    (output / 'runtime.sav').write_bytes(save('GraphicsRuntimeStateSave', {
        'SchemaVersion': 1, 'RequestedRevision': 22, 'SessionId': 345,
        'Phase': 2, 'ErrorCode': 0,
    }))

if __name__ == '__main__':
    main()
