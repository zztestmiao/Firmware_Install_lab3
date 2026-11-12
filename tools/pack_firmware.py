"""生成LED教学固件包，不烧录设备。所有32位字段采用小端序。"""
from pathlib import Path
import struct
import zlib

MAGIC = 0x3141544F
TARGET = 0x08010000

def pack(image, version):
    if not 8 <= len(image) <= 0x30000:
        raise ValueError("固件长度超出教学App范围")
    sp, entry = struct.unpack_from("<II", image)
    if not (0x20000000 < sp <= 0x20020000 and sp % 8 == 0 and
            entry & 1 and TARGET <= (entry & ~1) < TARGET + len(image)):
        raise ValueError("固件入口不符合当前LED布局")
    prefix = struct.pack("<7I", MAGIC, 1, version, TARGET, len(image),
                         zlib.crc32(image), 0)
    return prefix + struct.pack("<I", zlib.crc32(prefix)) + image

def validate(data):
    if len(data) < 40:
        raise ValueError("包过短")
    magic, fmt, version, target, length, crc, reserved, hcrc = struct.unpack_from("<8I", data)
    if magic != MAGIC or fmt != 1 or reserved != 0 or target != TARGET:
        raise ValueError("包格式或目标不匹配")
    if zlib.crc32(data[:28]) != hcrc or length != len(data) - 32:
        raise ValueError("包头校验或长度不匹配")
    image = data[32:]
    if zlib.crc32(image) != crc:
        raise ValueError("固件CRC不匹配")
    pack(image, version)  # 复用入口和容量检查
    return version, length, crc

if __name__ == "__main__":
    root = Path(__file__).resolve().parents[1]
    image = (root / "firmware/LED_V2.bin").read_bytes()
    data = pack(image, 2)
    result = validate(data)
    for bad in [data[:-1], data[:40] + bytes([data[40] ^ 1]) + data[41:],
                bytes([data[0] ^ 1]) + data[1:]]:
        try:
            validate(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("损坏包未被拒绝")
    (root / "firmware/LED_V2.ota").write_bytes(data)
    print("version=%d image_bytes=%d crc32=%08X package_bytes=%d" % (*result, len(data)))
    print("Valid package and three corruption checks passed")
