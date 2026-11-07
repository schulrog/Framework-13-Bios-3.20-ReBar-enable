"""Generate minimal synthetic IFR for parser tests; no firmware data is used."""
from pathlib import Path
import struct
import uuid

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'tests' / 'fixtures'
OUT.mkdir(parents=True, exist_ok=True)

def guid(text):
    return uuid.UUID(text).bytes_le

def make(hidden):
    advanced = guid('c6d4769e-7f48-4d2a-98e9-87adccf35ccc')
    sa = guid('72c5e28c-7783-43a1-8767-fad73fccafa4')
    formset = struct.pack('<BB16sHHB', 0x0e, 0x97, advanced, 0x1c92 if hidden else 0x1cbe, 0, 0)
    store = struct.pack('<BBH16sIH8s', 0x26, 34, 2, sa, 7, 0x578, b'SaSetup\0')
    root = struct.pack('<BBHH', 1, 0x86, 1, 0x1cbe) + b'\x29\x02'
    page = struct.pack('<BBHH', 1, 0x86, 0x159a, 1)
    question = struct.pack('<BBHHHHHBBBBB', 5, 0x91, 1, 2, 0x7cb if hidden else 0x448, 2, 0x42b, 0x10, 0x10, 0, 2, 0)
    options = b''.join(struct.pack('<BBHBBB', 9, 7, i + 1, 0x30 if i == 2 else 0, 0, i) for i in range(3))
    opcodes = formset + store + root + page + question + options + b'\x29\x02' * 3
    forms = struct.pack('<I', (2 << 24) | (4 + len(opcodes))) + opcodes
    end = struct.pack('<I', (0xdf << 24) | 4)
    return advanced + struct.pack('<I', 20 + len(forms) + len(end)) + forms + end

for name, hidden in [('normal', False), ('hidden', True)]:
    (OUT / f'{name}.bin').write_bytes(make(hidden))
print('Generated two synthetic parser fixtures; no BIOS input required.')
