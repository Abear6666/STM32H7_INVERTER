#!/usr/bin/env python3
import argparse
import binascii
import pathlib
import struct


APP_TAG_MAGIC = 0x41505447
APP_TAG_SIZE = 64
APP_SLOT_HEADER_SIZE = 0x400


def parse_u32(text: str) -> int:
    return int(text, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description="Create Apollo H743 App slot image with app_tag_t")
    parser.add_argument("--input", required=True, help="Raw app binary linked at app_run_addr")
    parser.add_argument("--output", required=True, help="Slot image: 0x400 header + app binary")
    parser.add_argument("--tag-output", required=True, help="Standalone app_tag_t binary")
    parser.add_argument("--run-addr", type=parse_u32, required=True)
    parser.add_argument("--version", type=parse_u32, required=True)
    parser.add_argument("--max-size", type=parse_u32, required=True)
    args = parser.parse_args()

    input_path = pathlib.Path(args.input)
    output_path = pathlib.Path(args.output)
    tag_output_path = pathlib.Path(args.tag_output)

    app = input_path.read_bytes()
    app_size = len(app)
    if app_size == 0:
        raise SystemExit("app binary is empty")
    if app_size > args.max_size:
        raise SystemExit(f"app binary too large: {app_size} > {args.max_size}")

    crc32 = binascii.crc32(app) & 0xFFFFFFFF
    tag = struct.pack(
        "<6I32s2I",
        APP_TAG_MAGIC,
        APP_TAG_SIZE,
        args.run_addr,
        app_size,
        args.version,
        crc32,
        bytes(32),
        0,
        0,
    )
    if len(tag) != APP_TAG_SIZE:
        raise SystemExit(f"internal tag size mismatch: {len(tag)}")

    header = bytearray([0xFF] * APP_SLOT_HEADER_SIZE)
    header[APP_SLOT_HEADER_SIZE - APP_TAG_SIZE:APP_SLOT_HEADER_SIZE] = tag

    output_path.parent.mkdir(parents=True, exist_ok=True)
    tag_output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(bytes(header) + app)
    tag_output_path.write_bytes(tag)

    print(f"app_size={app_size}")
    print(f"app_crc32=0x{crc32:08X}")
    print(f"app_run_addr=0x{args.run_addr:08X}")
    print(f"version={args.version}")
    print(f"slot_image={output_path}")
    print(f"tag={tag_output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
