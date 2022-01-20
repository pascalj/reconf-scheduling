#include "util.hpp"
#include "scheduling.hpp"

Graph import_task_graph(const nlohmann::json &j)
{
  const size_t ntasks = j["deps"].size();
  Graph        g(ntasks);

  // Set task names
  for (size_t i = 0; i < ntasks; i++) {
    auto v    = boost::vertex(i, g);
    g[v].name = j["tasklabels"][i].get<std::string>();
    for (size_t c = 0; c < j["cost"][i].size(); c++) {
      if(j["cost"][i][c].type() == nlohmann::json::value_t::boolean) {
        g[v]._cost[c].reset();
      } else {
        g[v]._cost[c] = j["cost"][i][c].get<int>();
      }
    }
  }

  // Insert dependencies
  for (size_t from = 0; from < ntasks; from++) {
    for (size_t to = 0; to < ntasks; to++) {
      if (j["deps"][from][to]) {
        boost::add_edge(from, to, g);
      }
    }
  }

  return g;
}

Configurations import_configs(const nlohmann::json &j)
{
  std::vector<Configuration> confs;
  for (auto conf : j["C"]["set"]) {
    Configuration c{conf["e"].get<std::string>()};
    size_t        i = 0;
    for (auto pe : j["P_config"]) {
      if (pe == conf["e"]) {
        c.add_pe(i);
      }
      i++;
    }
    confs.push_back(c);
  }

  return confs;
}

void export_svg(const Schedule &S, const std::string &filename)
{
  using namespace svg;

  auto max = std::max_element(
      S.pe_t_f.begin(), S.pe_t_f.end(), [](const auto &lhs, const auto &rhs) {
        return lhs.second < rhs.second;
      });

  Point       p_origin(10, 10);
  const int   x_scale    = 50;
  const int   min_height = 12;
  const float y_scale =
      1.0 / (std::transform_reduce(
                 S.scheduled_tasks.begin(),
                 S.scheduled_tasks.end(),
                 std::numeric_limits<float>::infinity(),
                 [](float lhs, float acc) { return std::min(lhs, acc); },
                 [](auto task) { return static_cast<float>(task.cost()); }) /
             min_height);
  const int pes = S.pe_t_f.size();

  Dimensions dimensions(
      p_origin.x + pes * x_scale,
      p_origin.y + max->second * y_scale);
  Document doc(filename, Layout(dimensions, Layout::TopLeft));

  size_t c_index = 2;
  int reconf_index = 1;
  for(auto reconfig : S.reconfigs) {
    Point reconf_origin(p_origin.x, reconfig * y_scale);
    Point text_origin(reconf_origin.x + 1, reconf_origin.y + 10);
    doc << Rectangle(
        reconf_origin,
        x_scale * pes,
        rho * y_scale,
        Fill(Color::Yellow));
    doc << Text(
        text_origin,
        "Reconfig #" + std::to_string(reconf_index++),
        Color::Black,
        Font(10, "Verdana"));
  }

  for (auto c : S.confs) {
    for (auto pe : c.pes) {
      auto tasks = S.tasks_on_pe(pe);
      for (auto task : tasks) {
        Point task_origin(p_origin.x, task.t_s() * y_scale);
        Point text_origin(task_origin.x + 1, task_origin.y + 10);
        doc << Rectangle(
            task_origin,
            x_scale,
            task.cost() * y_scale,
            Fill(),
            Stroke(1, static_cast<Color::Defaults>(c_index)));
        doc << Text(
            text_origin,
            task.vertex().name,
            Color::Black,
            Font(10, "Verdana"));
      }
      p_origin.x += x_scale;
    }
    c_index++;
  }

  doc.save();
}
