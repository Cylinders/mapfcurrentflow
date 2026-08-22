#!/usr/bin/env bash

set -euo pipefail

SOURCE="https://github.com/microsoft/onnxruntime/releases/download/v1.27.1/onnxruntime-linux-x64-gpu_cuda12-1.27.1.tgz"

ARCHIVE="onnxruntime.tgz"
DEST_DIR="onnxruntime"

echo "downloading ONNX Runtime..."
wget -O "$ARCHIVE" "$SOURCE"

echo "extracting..."
mkdir -p "$DEST_DIR"
tar -xzf "$ARCHIVE" -C "$DEST_DIR" --strip-components=1

echo "cleaning up..."
rm "$ARCHIVE"

echo "done"
echo "installed to $DEST_DIR"