#!/bin/bash

echo "Ray Tracer Performance Benchmark"
echo "================================="
echo ""

RAYTRACER="./build/bin/RayTracer"

if [ ! -f "$RAYTRACER" ]; then
    echo "Error: RayTracer executable not found. Please run ./build.sh first."
    exit 1
fi

echo "System Info:"
echo "CPU: $(lscpu | grep 'Model name' | cut -d':' -f2 | xargs)"
echo "Cores: $(nproc)"
echo "Memory: $(free -h | grep Mem | awk '{print $2}')"
echo ""

# Benchmark different settings
echo "Running benchmarks..."
echo ""

echo "1. Quick Preview (Low Quality):"
time $RAYTRACER examples/scene1.scene -w 200 -h 150 -s 10 -o bench_quick.ppm
echo ""

echo "2. Medium Quality:"
time $RAYTRACER examples/scene1.scene -w 400 -h 300 -s 50 -o bench_medium.ppm
echo ""

echo "3. High Quality:"
time $RAYTRACER examples/scene1.scene -w 800 -h 600 -s 100 -o bench_high.ppm
echo ""

echo "4. Thread Scaling Test (400x300, 50 samples):"
for threads in 1 2 4 8; do
    echo "Testing with $threads threads..."
    time $RAYTRACER examples/scene1.scene -w 400 -h 300 -s 50 -t $threads -o bench_thread_$threads.ppm
done
echo ""

echo "5. Complex Scene (BVH Performance):"
time $RAYTRACER examples/advanced_scene.scene -w 400 -h 300 -s 25 -o bench_complex.ppm
echo ""

echo "Benchmark complete! Output files generated:"
ls -lh bench_*.ppm

# Clean up benchmark files
read -p "Remove benchmark output files? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -f bench_*.ppm
    echo "Benchmark files removed."
fi
