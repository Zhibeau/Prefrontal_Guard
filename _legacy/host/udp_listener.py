#!/usr/bin/env python3
"""
Receive Aegis-Chip UDP steering vectors and emit console + JSON output.

Payload format:
  32 bytes = 16 signed INT16 values, little-endian.
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import sys
import time
from pathlib import Path

DIM = 16
DEFAULT_IP = "0.0.0.0"
DEFAULT_PORT = 1234


def decode_vector(payload: bytes) -> list[int]:
    if len(payload) < DIM * 2:
        raise ValueError(f"expected at least 32 payload bytes, got {len(payload)}")
    return list(struct.unpack("<" + "h" * DIM, payload[: DIM * 2]))


def build_prompt(vector: list[int], source_ip: str, source_port: int) -> str:
    return (
        "Aegis-Chip steering vector received from FPGA.\n"
        f"Source: {source_ip}:{source_port}\n"
        f"Interference vector (INT16, little-endian): {vector}\n"
        "Inject this vector into the LLM steering hook for the current token step."
    )


def emit_record(vector: list[int], addr: tuple[str, int], payload_len: int) -> dict:
    ts = time.time()
    return {
        "timestamp": ts,
        "source_ip": addr[0],
        "source_port": addr[1],
        "payload_len": payload_len,
        "vector_format": "int16-le",
        "dimension": DIM,
        "vector": vector,
        "prompt": build_prompt(vector, addr[0], addr[1]),
    }


def append_jsonl(path: Path, record: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(record, ensure_ascii=False) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Listen for Aegis FPGA UDP steering vectors")
    parser.add_argument("--bind-ip", default=DEFAULT_IP, help="local IP to bind (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="UDP port to bind (default: 1234)")
    parser.add_argument("--jsonl", default=None, help="optional JSONL file to append decoded records")
    parser.add_argument("--once", action="store_true", help="exit after the first valid vector")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((args.bind_ip, args.port))

    print(f"Listening on {args.bind_ip}:{args.port} for Aegis UDP vectors...")

    try:
        while True:
            payload, addr = sock.recvfrom(2048)
            try:
                vector = decode_vector(payload)
            except ValueError as exc:
                print(f"Skipping packet from {addr[0]}:{addr[1]}: {exc}", file=sys.stderr)
                continue

            record = emit_record(vector, addr, len(payload))

            print("\n=== Aegis UDP vector ===")
            print(f"from     : {record['source_ip']}:{record['source_port']}")
            print(f"bytes    : {record['payload_len']}")
            print(f"vector   : {record['vector']}")
            print("prompt   :")
            print(record["prompt"])
            print("json     :")
            print(json.dumps(record, ensure_ascii=False))

            if args.jsonl:
                append_jsonl(Path(args.jsonl), record)

            if args.once:
                break
    finally:
        sock.close()


if __name__ == "__main__":
    main()