#!/usr/bin/env python3
import argparse
import binascii
import pathlib
import sys
import time


def parse_u32(text: str) -> int:
    return int(text, 0)


def read_until(ser, needle: bytes, timeout_s: float) -> bytes:
    deadline = time.monotonic() + timeout_s
    data = bytearray()

    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if chunk:
            data.extend(chunk)
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            if needle in data:
                return bytes(data)
        else:
            time.sleep(0.01)

    raise TimeoutError(f"timeout waiting for {needle!r}")


def wait_for_app_ready(ser, timeout_s: float, probe_interval_s: float, probe: bool) -> None:
    deadline = time.monotonic() + timeout_s
    next_probe = 0.0
    data = bytearray()
    markers = (b"task_iap started", b"IAP: serial local source ready", b"IAP status:")

    while time.monotonic() < deadline:
        now = time.monotonic()
        if probe and now >= next_probe:
            ser.write(b"iap status\n")
            ser.flush()
            next_probe = now + probe_interval_s

        chunk = ser.read(256)
        if chunk:
            data.extend(chunk)
            if len(data) > 4096:
                del data[:len(data) - 4096]
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            if any(marker in data for marker in markers):
                return
        else:
            time.sleep(0.02)

    raise TimeoutError("timeout waiting for App IAP task")


def drain_logs(ser, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s

    while time.monotonic() < deadline:
        chunk = ser.read(256)
        if chunk:
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
        else:
            time.sleep(0.02)


def main() -> int:
    parser = argparse.ArgumentParser(description="Send an App image to Apollo H743 serial IAP staging")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--file", required=True, help="Binary package, usually build/gcc-debug/app_a_slot.bin")
    parser.add_argument("--version", type=parse_u32, default=2)
    parser.add_argument("--chunk", type=int, default=128)
    parser.add_argument("--delay", type=float, default=0.002, help="Delay between chunks in seconds")
    parser.add_argument("--app-timeout", type=float, default=60.0)
    parser.add_argument("--probe-interval", type=float, default=1.0)
    parser.add_argument("--probe-app", action="store_true", help="Send iap status probes while waiting")
    parser.add_argument("--no-wait-app", action="store_true")
    parser.add_argument("--ready-timeout", type=float, default=20.0)
    parser.add_argument("--log-timeout", type=float, default=8.0)
    args = parser.parse_args()

    try:
        import serial
    except ImportError as exc:
        raise SystemExit("pyserial is required: python -m pip install pyserial") from exc

    path = pathlib.Path(args.file)
    image = path.read_bytes()
    if not image:
        raise SystemExit("input file is empty")
    if args.chunk <= 0:
        raise SystemExit("--chunk must be positive")

    crc32 = binascii.crc32(image) & 0xFFFFFFFF
    command = f"iap recv {len(image)} 0x{crc32:08X} {args.version}\r\n".encode("ascii")

    print(f"serial={args.port} baud={args.baud}")
    print(f"file={path} size={len(image)} crc32=0x{crc32:08X} version={args.version}")

    with serial.Serial(args.port, args.baud, timeout=0.05, write_timeout=5.0) as ser:
        ser.dtr = False
        ser.rts = False
        ser.reset_input_buffer()
        if not args.no_wait_app:
            wait_for_app_ready(ser, args.app_timeout, args.probe_interval, args.probe_app)

        ser.write(command)
        ser.flush()
        print(command.decode("ascii").strip())
        read_until(ser, b"ready for binary", args.ready_timeout)

        sent = 0
        while sent < len(image):
            end = min(sent + args.chunk, len(image))
            ser.write(image[sent:end])
            ser.flush()
            sent = end
            if args.delay > 0:
                time.sleep(args.delay)

        print(f"\nsent={sent}")
        drain_logs(ser, args.log_timeout)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
