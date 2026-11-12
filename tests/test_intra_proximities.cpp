#include <binding_region.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

static void require_equal(const std::vector<int> &got,
                          const std::vector<int> &want,
                          const char *description)
{
  if (got != want) {
    std::cerr << "FAIL: " << description << '\n';
    std::exit(EXIT_FAILURE);
  }
}

static std::vector<int> bounded_candidates(binding_region &region,
    std::vector<vec> &coordinates, int hinge, int direction, int extension,
    vec anchor_coordinate, double radius, int anchor = -1, int min_distance = 0)
{
  region.update_intra_proximities(extension, hinge, direction, radius,
                                 anchor_coordinate, coordinates);
  if (anchor >= 0) region.filter_proximities_near_a(min_distance, anchor);
  return region.get_and_filter_intra_candidates(extension, hinge, direction);
}

int main()
{
  std::ostringstream log;
  std::streambuf *stdout_buffer = std::cout.rdbuf(log.rdbuf());
  {
    std::vector<vec> coordinates(8, vec{0.0, 0.0, 0.0});
    binding_region linear("linear", 0, 7, 8, false, false, -1, -1);
    linear.prepare_idx();
    require_equal(bounded_candidates(linear, coordinates, 3, 1, 3,
                                    {0, 0, 0}, 10, 1),
                  {3, 4, 5, 6}, "candidate order");
    require_equal(bounded_candidates(linear, coordinates, 6, 1, 5,
                                    {0, 0, 0}, 10),
                  {6, 7}, "linear endpoint is not repeated");
    require_equal(bounded_candidates(linear, coordinates, 6, -1, 5,
                                    {0, 0, 0}, 10, 3, 1),
                  {6, 5}, "anchor exclusion stops the candidate path");
    binding_region circular("circle", 0, 7, 8, true, false, -1, -1);
    circular.prepare_idx();
    require_equal(bounded_candidates(circular, coordinates, 6, 1, 12,
                                    {0, 0, 0}, 10),
                  {6, 7, 0, 1, 2, 3, 4, 5}, "circle stops before revisiting hinge");
    require_equal(bounded_candidates(circular, coordinates, 6, 1, 12,
                                    {0, 0, 0}, 10, 1),
                  {6, 7, 0}, "anchor exclusion across the circular boundary");
    coordinates[4].x = 1.0;
    require_equal(bounded_candidates(linear, coordinates, 3, 1, 3,
                                    {0, 0, 0}, 1.0),
                  {3}, "grab radius uses a strict cutoff");
    coordinates[4].x = std::nextafter(1.0, 0.0);
    coordinates[5].x = std::numeric_limits<double>::quiet_NaN();
    require_equal(bounded_candidates(linear, coordinates, 3, 1, 3,
                                    {0, 0, 0}, 1.0),
                  {3, 4}, "immediate inside-cutoff point and NaN stop");
    require_equal(bounded_candidates(linear, coordinates, 3, 1, 0,
                                    {0, 0, 0}, 1.0),
                  {3}, "zero extension retains the existing hinge");
  }

  std::mt19937 random(90210);
  std::uniform_real_distribution<double> coordinate(-2.0, 2.0);
  std::size_t checked = 0;
  for (int shape = 0; shape < 3; ++shape) {
    const bool crossing = shape == 2;
    const int size = crossing ? 44 : 97;
    binding_region reference("reference", crossing ? 12 : 13,
        crossing ? 80 : 109, size, shape == 1, crossing, 0, 110);
    reference.prepare_idx();
    binding_region bounded(reference);
    std::vector<vec> coordinates(128);
    for (int trial = 0; trial < 6000; ++trial) {
      for (vec &position : coordinates)
        position = {coordinate(random), coordinate(random), coordinate(random)};
      const int hinge = reference.get_mono_pos(random() % size);
      const int anchor = reference.get_mono_pos(random() % size);
      const bool same_region = (random() % 2) == 0;
      const int min_distance = random() % size;
      const int extension = random() % (2 * size);
      const int direction = (random() % 2) ? 1 : -1;
      const double radii[] = {0.0, 0.5, 1.0, 2.0, 20.0};
      const double radius = radii[random() % 5];
      const vec anchor_coordinate = same_region ? coordinates[anchor]
                                                : vec{0.25, -0.5, 0.75};
      // Give unvisited entries a mix of stale values before each bounded update.
      bounded.update_proximities((trial % 2) ? 0.0 : 20.0,
                                  anchor_coordinate, coordinates);
      reference.update_proximities(radius, anchor_coordinate, coordinates);
      if (same_region) reference.filter_proximities_near_a(min_distance, anchor);
      const std::vector<int> expected =
          reference.get_and_filter_intra_candidates(extension, hinge, direction);
      require_equal(bounded_candidates(bounded, coordinates, hinge, direction,
          extension, anchor_coordinate, radius, same_region ? anchor : -1,
          min_distance), expected, "bounded search differs from full scan");
      ++checked;
    }
  }
  std::cout.rdbuf(stdout_buffer);
  std::cout << "PASS: explicit cutoff/boundary cases and " << checked
            << " randomized full-scan comparisons\n";
}
