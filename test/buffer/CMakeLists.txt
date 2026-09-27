minitub_add_test(lru_k_replacer_test lru_k_replacer_test.cpp)
minitub_add_test(buffer_pool_manager_test buffer_pool_manager_test.cpp)
# 8 threads x 10k ops with real disk I/O; slow under TSan.
minitub_add_test(buffer_pool_stress_test buffer_pool_stress_test.cpp TIMEOUT 300)
