#include <loop_topology.hpp>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

// Exercise the real topology, candidate selection and random draws.  The runner
// compares this output against the same driver built with the pre-change source.
int main()
{
  std::ostringstream log;
  std::ostringstream states;
  std::streambuf *stdout_buffer = std::cout.rdbuf(log.rdbuf());
  for (int shape = 0; shape < 4; ++shape) {
    for (int seed : {1, 7, 42, 1234}) {
      for (bool unbind : {false, true}) {
        const int count = 1024;
        std::vector<int> types(count, 3);
        int *type_data = types.data();
        std::vector<std::string> leaves{"root"};
        std::vector<theta_topo> leaf_topologies{
            {0, count - 1, count - 1, 0, count / 2}};
        if (shape == 1) {
          leaf_topologies[0].start_link = 10;
          leaf_topologies[0].end_link = 11;
        } else if (shape == 2) {
          leaves = {"left", "right"};
          leaf_topologies = {{0, 511, 511, 0, 256},
                             {512, 1023, 1023, 512, 768}};
        } else if (shape == 3) {
          types[256] = types[768] = 6;
        }
        std::vector<vec> coordinates(count);
        loop_topology topology;
        topology.prepare_binding_regions(leaves, leaf_topologies, type_data);
        topology.set_step_dist("poisson", 20.0, 30);
        topology.prng_seed(seed);
        topology.initialize_loops(unbind ? 1 : 24, 6);
        // A single forced unbinding update checks the unchanged full search.
        // Longer default runs check repeated bound moves and RNG continuity.
        for (int step = 0; step < (unbind ? 1 : 100); ++step) {
          for (int i = 0; i < count; ++i) {
            const double phase = 0.031 * i + 0.001 * step;
            coordinates[i] = {300.0 * std::cos(phase),
                              300.0 * std::sin(phase),
                              100.0 * std::sin(phase * 0.37)};
          }
          topology.set_coords(coordinates);
          topology.update_loops(30, 6, unbind ? 1.0 : 0.0, 500.0);
          for (loop state : topology.get_loops()) {
            states << shape << ',' << seed << ',' << unbind << ',' << step
                   << ',' << state.get_a() << ',' << state.get_h()
                   << ',' << state.get_d() << ',' << state.get_a_region()
                   << ',' << state.get_h_region() << ',' << state.get_h_bound()
                   << '\n';
          }
        }
      }
    }
  }
  std::cout.rdbuf(stdout_buffer);
  std::cout << states.str();
}
