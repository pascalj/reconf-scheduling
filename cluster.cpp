#include <iostream>
#include "algorithms.hpp"
#include "scheduling.hpp"
#include "util.hpp"

int rho = 2;

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cout << "No input file given. Usage:" << std::endl
              << std::endl
              << "    cluster <inputjson>.json [rho]" << std::endl;
    return 1;
  }

  std::filesystem::path json_path(argv[1]);
  if (argc >= 3) {
    rho = atoi(argv[2]);
  }

  // Import
  std::ifstream  i(argv[1]);
  nlohmann::json j;
  i >> j;
  auto G = import_task_graph(j);
  auto C = import_configs(j);

  auto start = std::chrono::high_resolution_clock::now();
  auto s     = cluster(G, C);
  auto end   = std::chrono::high_resolution_clock::now();

  auto ms =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "cluster," << boost::num_vertices(G) << "," << s.makespan() << ","
            << ms.count() << std::endl;

  json_path.replace_extension("svg");
  export_svg(s, json_path.filename());

  return 0;
}
