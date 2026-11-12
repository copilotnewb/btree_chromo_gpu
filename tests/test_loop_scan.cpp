#include <loop_topology.hpp>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

// Count the actual distance evaluations, without changing their calculation.
// The test runner links this wrapper around the production v_L2 method.
static std::size_t distance_evaluations = 0;
extern "C" double __real__ZN20vec_quat_manipulator4v_L2E3vec(
    vec_quat_manipulator *, vec);
extern "C" double __wrap__ZN20vec_quat_manipulator4v_L2E3vec(
    vec_quat_manipulator *self, vec value)
{
  ++distance_evaluations;
  return __real__ZN20vec_quat_manipulator4v_L2E3vec(self, value);
}

int main()
{
  const int count = 54338;
  const int max_extension = 30;
  std::vector<int> types(count, 3);
  int *type_data = types.data();
  std::vector<vec> coordinates(count, vec{0.0, 0.0, 0.0});
  std::ostringstream log;
  std::streambuf *stdout_buffer = std::cout.rdbuf(log.rdbuf());
  {
    loop_topology topology;
    topology.prepare_binding_regions({"root"},
        {{0, count - 1, count - 1, 0, count / 2}}, type_data);
    topology.set_step_dist("poisson", 20.0, max_extension);
    topology.prng_seed(42);
    topology.initialize_loops(1, 6);
    topology.set_coords(coordinates);
    distance_evaluations = 0;
    topology.update_loops(max_extension, 6, 0.0, 500.0);
  }
  std::cout.rdbuf(stdout_buffer);
  if (distance_evaluations > static_cast<std::size_t>(max_extension)) {
    std::cerr << "FAIL: a bound hinge evaluated " << distance_evaluations
              << " distances for at most " << max_extension
              << " candidate steps\n";
    return EXIT_FAILURE;
  }
  std::cout << "PASS: bound hinge evaluated " << distance_evaluations
            << " distances (limit " << max_extension << ")\n";
  return EXIT_SUCCESS;
}
