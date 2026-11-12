#include <binding_region.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

int main()
{
  const int count = 54338;
  const int queries = 500;
  const int extension = 30;
  std::vector<vec> coordinates(count);
  for (int i = 0; i < count; ++i) {
    const double angle = i * 0.01;
    coordinates[i] = {200 * std::cos(angle), 200 * std::sin(angle),
                      50 * std::sin(angle * 0.31)};
  }
  std::ostringstream log;
  std::streambuf *stdout_buffer = std::cout.rdbuf(log.rdbuf());
  std::vector<double> full_times, bounded_times;
  std::uint64_t full_checksum = 0, bounded_checksum = 0;
  {
    binding_region full("full", 0, count - 1, count, true, false, -1, -1);
    full.prepare_idx();
    binding_region bounded(full);
    for (int repeat = 0; repeat < 5; ++repeat) {
      auto begin = std::chrono::steady_clock::now();
      for (int query = 0; query < queries; ++query) {
        const int hinge = 1000 + query;
        full.update_proximities(500.0, coordinates[100], coordinates);
        full.filter_proximities_near_a(6, 100);
        for (int candidate : full.get_and_filter_intra_candidates(extension, hinge, 1))
          full_checksum += candidate;
      }
      auto end = std::chrono::steady_clock::now();
      full_times.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
      begin = std::chrono::steady_clock::now();
      for (int query = 0; query < queries; ++query) {
        const int hinge = 1000 + query;
        bounded.update_intra_proximities(extension, hinge, 1, 500.0,
                                         coordinates[100], coordinates);
        bounded.filter_proximities_near_a(6, 100);
        for (int candidate : bounded.get_and_filter_intra_candidates(extension, hinge, 1))
          bounded_checksum += candidate;
      }
      end = std::chrono::steady_clock::now();
      bounded_times.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
    }
  }
  std::cout.rdbuf(stdout_buffer);
  if (full_checksum != bounded_checksum) {
    std::cerr << "FAIL: benchmark candidate checksums differ\n";
    return 1;
  }
  std::sort(full_times.begin(), full_times.end());
  std::sort(bounded_times.begin(), bounded_times.end());
  std::cout << std::fixed << std::setprecision(3)
            << "{\"monomers\":" << count << ",\"queries\":" << queries
            << ",\"max_extension\":" << extension
            << ",\"repeats\":5,\"full_scan_median_ms\":" << full_times[2]
            << ",\"bounded_scan_median_ms\":" << bounded_times[2]
            << ",\"microbenchmark_speedup\":" << full_times[2] / bounded_times[2]
            << ",\"candidate_checksum\":" << bounded_checksum << "}\n";
}
