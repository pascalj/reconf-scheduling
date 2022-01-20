#include <iostream>
#include <numeric>
#include "scheduling.hpp"
#include "boost/graph/topological_sort.hpp"

extern int rho;

PE::PE(size_t o)
  : offset(o)
{
}

bool PE::operator==(const PE &other) const
{
  return offset == other.offset;
}

std::optional<int> TaskV::cost(PE p) const
{
  return _cost[p.offset];
};

Configuration::Configuration(const std::string &n)
  : name(n)
{
}

bool Configuration::operator==(const Configuration &other) const
{
  return name == other.name;
}

PE Configuration::optimal_pe(const TaskV &v) const
{
  auto opt = std::min_element(
      pes.begin(), pes.end(), [&v](const auto lhs, const auto rhs) {
        // Note: operator<() does the exact oposite of this if one value is
        // missing. We want it to be +infty while since C++17 the missing
        // value is considered -infty.
        const auto l = v.cost(lhs);
        const auto r = v.cost(rhs);
        if (l && r) {
          return l.value() < r.value();
        }
        else if (l) {
          return true;
        }
        else {
          return false;
        }
      });
  return *opt;
}

std::optional<int> Configuration::min_cost(const TaskV &v) const
{
  auto opt_pe = optimal_pe(v);
  return v.cost(opt_pe);
}

// divide the cost by all available PEs (average / num_avail_pe)
std::optional<int> Configuration::divided_cost(const TaskV &v) const
{
  auto sum = std::accumulate(
      pes.begin(),
      pes.end(),
      0,
      [&v](const auto acc, const auto pe) {
        auto cost = v.cost(pe);
        if (cost) {
          return acc + cost.value();
        }
        else {
          return acc;
        }
      });

  auto count = std::count_if(pes.begin(), pes.end(), [&v](const auto pe) {
    return static_cast<bool>(v.cost(pe).has_value());
  });
  
  if (count > 0) {
    return std::optional<int>(sum / count / count);
  } else {
    return std::optional<int>{};
  }
}

void Configuration::add_pe(size_t offset)
{
  pes.emplace_back(offset);
}

Schedule::Schedule(const Configurations &C)
  : confs(C)
{
  for (auto c : confs) {
    for (auto &pe : c.pes) {
      pe_t_f[pe] = 0;
    }
  }
}
int Schedule::ScheduledTask::cost() const
{
  assert(task.cost(p));
  return task.cost(p).value();
}
int Schedule::ScheduledTask::t_f() const
{
  return cost() + t_s();
}

int Schedule::makespan() const
{
  auto max_pe = std::max_element(
      pe_t_f.begin(), pe_t_f.end(), [](const auto lhs, const auto rhs) {
        return lhs.second < rhs.second;
      });
  if (max_pe == pe_t_f.end()) {
    return 0;
  }

  return max_pe->second;
}

int Schedule::ScheduledTask::t_s() const
{
  return _t_s;
}

PE Schedule::ScheduledTask::pe() const
{
  return p;
};
TaskV Schedule::ScheduledTask::vertex() const {
  return task;
}

int Schedule::max_t_f(const PE &p) const
{
  if (pe_t_f.count(p) > 0) {
    return pe_t_f.find(p)->second;
  } else {
    return 0;
  }
}

std::pair<PE, int> Schedule::earliest_finish(const TaskV &v)
{
  auto min = std::min_element(
      pe_t_f.begin(), pe_t_f.end(), [&](const auto &lhs, const auto &rhs) {
        auto lhs_cost = v.cost(lhs.first);
        auto rhs_cost = v.cost(rhs.first);

        if(lhs_cost && rhs_cost) {
          return lhs_cost.value() < rhs_cost.value();
        } else if (lhs_cost) {
          return true;
        }
        return false;
      });

  assert(min != pe_t_f.end());
  assert(v.cost(min->first));

  return std::make_pair(min->first, min->second + v.cost(min->first).value());
}

std::pair<PE, int> Schedule::asap(const Configuration &C, const TaskV &v)
{
  auto min = std::min_element(
      C.pes.begin(), C.pes.end(), [&v,this](const auto &lhs, const auto &rhs) {
        auto lhs_cost = v.cost(lhs);
        auto rhs_cost = v.cost(rhs);

        if(lhs_cost && rhs_cost) {
          return pe_t_f[lhs] + lhs_cost.value() < pe_t_f[rhs] + rhs_cost.value();
        } else if (lhs_cost) {
          return true;
        }
        return false;
      });

  auto t_s = pe_t_f[*min] + 1;
  if(not reconfigs.empty()) {
    t_s = std::max(reconfigs.back(), t_s);
  }

  return std::make_pair(*min, t_s);
}

std::pair<PE, int> Schedule::earliest_finish(const TaskV& v, const Configuration &C)
{

  auto min = std::min_element(
      C.pes.begin(), C.pes.end(), [this,&v](const auto &lhs, const auto &rhs) {
        const auto lhs_cost = v.cost(lhs);
        const auto rhs_cost = v.cost(rhs);
        if(lhs_cost && rhs_cost) {
          return lhs_cost.value() + pe_t_f[lhs] < rhs_cost.value() + pe_t_f[rhs];
        } else if (lhs_cost) {
          return true;
        }
        return false;
      });

  return std::make_pair(*min, pe_t_f[*min]);
}



std::vector<Schedule::ScheduledTask> Schedule::tasks_on_pe(const PE &p) const
{
  std::vector<ScheduledTask> t;
  std::copy_if(
      scheduled_tasks.begin(), scheduled_tasks.end(), std::back_inserter(t), [&](auto mtask) {
        return mtask.pe() == p;
      });
  return t;
}

Schedule::ScheduledTask &Schedule::schedule_task(TaskV v, PE p, int t_s)
{
  scheduled_tasks.emplace_back(v, p, t_s);

  pe_t_f[p] = scheduled_tasks.back().t_f();
  return scheduled_tasks.back();
}
Schedule::ScheduledTask &Schedule::schedule_task(TaskV v, PE p)
{
  auto t_s = std::max(reconfigs.back() + rho, max_t_f(p));
  return schedule_task(v, p, t_s + 1);
}
int Schedule::insert_reconfiguration(int rho) {
  int limit = 0;
  for (auto c : confs) {
    for (auto pe : c.pes) {
      int max = max_t_f(pe);
      if(max > limit) {
        limit = max;
      }
    }
  }
  reconfigs.push_back(limit);
  return limit + rho;
}
int Schedule::t_f(TaskV v)
{
  auto task = std::find_if(scheduled_tasks.begin(), scheduled_tasks.end(), [&v](auto &task) {
      return task.vertex().name == v.name;
      });
  assert(task != scheduled_tasks.end());
  return task->t_f();
}


std::ostream &operator<<(std::ostream &os, const Schedule &S)
{
  for (auto c : S.confs) {
    for (auto pe : c.pes) {
      auto tasks = S.tasks_on_pe(pe);
      for (auto t : tasks) {
        os << "[" << t.t_s() << "-" << t.t_f() << "] " << t.vertex().name << std::endl;
      }
      os << std::endl;
    }
    os << std::endl;
  }

  return os;
}

