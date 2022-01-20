#pragma once

#include <stddef.h>
#include <vector>
#include <unordered_map>
#include <optional>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/subgraph.hpp>

constexpr size_t MaxPE = 7;

class PE {
  public:
  size_t offset;

  explicit PE(size_t o);

  bool operator==(const PE &other) const;

  struct Hash {
    std::size_t operator()(const PE &p) const noexcept
    {
      return p.offset;
    }
  };
};

struct TaskV {
  std::string            name;
  std::array<std::optional<int>, MaxPE> _cost;

  std::optional<int> cost(PE p) const;
};

struct Configuration {
  std::string     name;
  std::vector<PE> pes;

  explicit Configuration(const std::string &n);
  bool               operator==(const Configuration &other) const;
  PE                 optimal_pe(const TaskV &v) const;
  std::optional<int> min_cost(const TaskV &v) const;
  std::optional<int> divided_cost(const TaskV &v) const;
  void               add_pe(size_t offset);
};
using Configurations = std::vector<Configuration>;

// Mapping of PE[n] to config
using PEs = std::vector<PE>;

using Graph  = boost::subgraph<boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS,
    TaskV,
    boost::property<boost::edge_index_t, int>>>;
using Vertex = Graph::vertex_descriptor;

struct Schedule {
  struct ScheduledTask {
    ScheduledTask(const TaskV &v, const PE &pe, int start)
      : task(v)
      , p(pe)
      , _t_s(start)
    {
      assert(task.cost(pe));
    }

    int cost() const;
    int t_s() const;
    int t_f() const;
    PE  pe() const;
    TaskV vertex() const;

  private:
    TaskV task;
    PE    p;
    int   _t_s;
  };

public:
  explicit Schedule(const Configurations &C);
  int                        max_t_f(const PE &p) const;
  std::vector<ScheduledTask> tasks_on_pe(const PE &p) const;
  ScheduledTask             &schedule_task(TaskV v, PE p, int t_s);
  ScheduledTask             &schedule_task(TaskV v, PE p);
  int                        insert_reconfiguration(int rho);
  int                        t_f(TaskV v);
  int                        makespan() const;
  std::pair<PE, int>         earliest_finish(const TaskV &);
  std::pair<PE, int>         asap(const Configuration &, const TaskV &);
  std::pair<PE, int> earliest_finish(const TaskV &, const Configuration &);

  friend std::ostream &operator<<(std::ostream &os, const Schedule &S);

  std::vector<ScheduledTask>            scheduled_tasks;
  Configurations                        confs;
  std::vector<int>                      reconfigs;
  std::unordered_map<PE, int, PE::Hash> pe_t_f;
};
