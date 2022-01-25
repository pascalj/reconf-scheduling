#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "json.hpp"

#include "algorithms.hpp"
#include "scheduling.hpp"
#include "util.hpp"

int rho = 2;

int main(int argc, char **argv)
{
  int L = 3;
  if (argc < 2) {
    std::cout << "No input file given. Usage:" << std::endl
              << std::endl
              << "    schedule <inputjson>.mzn [rho] [L]" << std::endl;
    return 1;
  }

  std::filesystem::path json_path(argv[1]);
  if(argc >= 3) {
    rho = atoi(argv[2]);
  }
  if(argc >= 4) {
    L = atoi(argv[3]);
  }

  // Import
  std::ifstream  i(json_path);
  nlohmann::json j;
  i >> j;
  auto G = import_task_graph(j);
  auto C = import_configs(j);

  auto start = std::chrono::high_resolution_clock::now();
  auto s = lsl(G, C, L);
  auto end = std::chrono::high_resolution_clock::now();

  auto ms =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "lsl," << rho << "," << boost::num_vertices(G) << "," << s.makespan() << ","
            << ms.count() << "," << s.reconfigs.size() << "," << L << std::endl;

  json_path.replace_extension("svg");

  export_svg(s, json_path.filename());

  return 0;
}
