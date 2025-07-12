#!/bin/bash

echo "Building the Docker image..."
docker build -t growth-asymmetry-builder .

echo ""
echo "Compiling the C++ code inside the container..."
docker run --rm -v "$(pwd)":/app growth-asymmetry-builder /bin/bash -c "mkdir -p build && cd build && cmake .. && make"

if [ $? -eq 0 ]; then
  echo ""
  echo "Build successful! The 'stock_screener' executable is in the 'build' directory."
else
  echo ""
  echo "Build failed. Please check the compilation errors above."
fi