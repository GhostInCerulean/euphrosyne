add_executable(euphrosyne_benchmarks
        glm_vs_hlslpp.cpp
)

target_link_libraries(euphrosyne_benchmarks PRIVATE
        euphrosyne
        benchmark::benchmark
        benchmark::benchmark_main
)