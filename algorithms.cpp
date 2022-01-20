#include <stddef.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <cassert>
#include <iostream>
#include <iterator>

#include "algorithms.hpp"
#include "scheduling.hpp"

#include <boost/graph/subgraph.hpp>
#include <boost/graph/topological_sort.hpp>
#include <numeric>

Schedule lsl(const Graph &g, const Configurations &C, size_t L)
{
  Schedule            S(C);
  std::vector<Vertex> sorted_g;
  boost::topological_sort(g, std::back_inserter(sorted_g));

  auto c_current     = C.begin();
  auto c_last        = C.end();
  int  last_reconfig = 0;

  for (size_t i = 0; i < sorted_g.size(); i++) {
    // distance from current configuration
    std::vector<std::optional<int>> c_distance(C.size(), rho);
    c_distance[std::distance(C.begin(), c_current)] = 0;

    // accumulate the cost for each configuration
    std::transform(
        C.begin(),
        C.end(),
        c_distance.begin(),
        c_distance.begin(),
        [&](const auto c, const auto acc) {
          if (!acc) {
            return acc;
          }

          auto tmp_acc = acc;
          for (size_t offset = i; offset < std::min(i + L, sorted_g.size());
               offset++) {
            auto task         = g[sorted_g[offset]];
            auto current_cost = c_current->min_cost(task);
            auto other_cost   = c.min_cost(task);

            if (offset == i && (!current_cost && !other_cost)) {
                return std::optional<int>();
            }

            // When we use both of these configurations, this will be the cost
            // at minimum.
            if (current_cost || other_cost) {
              tmp_acc = tmp_acc.value() + std::min(
                                              current_cost.value_or(INT_MAX),
                                              other_cost.value_or(INT_MAX));
            }
            else {
              // Or we cannot execute the tasks at all.
              return std::optional<int>{};
            }
          }
          return tmp_acc;
        });

    // select the configuration with minimal cost
    auto best_config = std::min_element(
        c_distance.begin(),
        c_distance.end(),
        [](const auto lhs, const auto rhs) {
          if (lhs && rhs) {
            return lhs.value() < rhs.value();
          }
          else if (lhs) {
            return true;
          }
          return false;
        });


    auto task     = g[sorted_g[i]];

    auto c_next = std::next(
          C.begin(), std::distance(c_distance.begin(), best_config));

    // If the next configuration supports the current task, switch.
    if(c_next->min_cost(task)) {
      c_current = c_next;
    }


    auto asap     = S.earliest_finish(task, *c_current);
    auto best_t_s = asap.second;
    auto preds    = adjacent_vertices(sorted_g[i], g);

    // find the latest t_f of all predecessors
    best_t_s = std::transform_reduce(
        preds.first,
        preds.second,
        best_t_s,
        [](const auto lhs, const auto rhs) { return std::max(lhs, rhs); },
        [&](const auto pred) { return S.t_f(g[pred]); });

    if (c_current != c_last) {
      c_last        = c_current;
      last_reconfig = S.insert_reconfiguration(rho);
    }
    S.schedule_task(task, asap.first, std::max(best_t_s, last_reconfig) + 1);
  }

  return S;
}

Schedule cluster(Graph &g, Configurations &C)
{
  Schedule   S(C);
  Clustering clustering(std::move(g), C);

  // Assign initial cost and configurations to clusters

  bool done = false;
  while (not done) {
    done = clustering.merge();
  }

  auto last_reconfig = 0;
  for (auto cluster : clustering.clusters) {
    if (not cluster.is_empty()) {
      last_reconfig = S.insert_reconfiguration(rho);
    }
    for (auto it = cluster.front; it != cluster.back; ++it) {
      auto& task = clustering.graph[*it];
      auto asap = S.asap(*cluster.config, task);
 
      // find the latest t_f of all predecessors
      auto preds    = adjacent_vertices(*it, clustering.graph);
      auto best_t_s = asap.second;
      best_t_s      = std::transform_reduce(
          preds.first,
          preds.second,
          best_t_s,
          [](const auto lhs, const auto rhs) { return std::max(lhs, rhs); },
          [&](const auto pred) { return S.t_f(g[pred]) + 1; });
      S.schedule_task(clustering.graph[*it], asap.first, std::max(best_t_s, last_reconfig + 1));
    }
  }

  return S;
}

Clustering::Clustering(Graph &&g, const Configurations &configs)
  : graph(std::move(g))
  , C(configs)
{
  boost::topological_sort(g, std::back_inserter(order));
  for (auto it = order.begin(); it != order.end(); it++) {
    Cluster new_cluster(it, it + 1);
    auto    best_config = opt_cluster_cost(new_cluster);
    new_cluster.config  = std::addressof(*best_config.first);
    new_cluster.cost    = best_config.second;
    clusters.push_back(std::move(new_cluster));
  }
}

Cluster::Cluster()
  : front(OrderIt{})
  , back(std::next(front))
{
}

Cluster::Cluster(OrderIt f, OrderIt b)
  : front(f)
  , back(b)
{
}

Cluster::Cluster(OrderIt f, OrderIt b, const Configuration &c, int _cost)
  : front(f)
  , back(b)
  , cost(_cost)
  , config(std::addressof(c))
{
}

Cluster Cluster::expand(const Cluster &other) const
{
  Cluster copy = *this;

  copy.back   = other.back;
  copy.cost   = 0;
  copy.config = nullptr;

  return copy;
}

void Cluster::empty()
{
  front = back;
  cost  = 0;
}

bool Cluster::is_empty() const
{
  return front == back;
}

std::optional<int> Clustering::cluster_cost(
    const Configuration &c, const Cluster &cluster) const
{
  assert(std::distance(cluster.front, cluster.back) >= 1);
  return std::transform_reduce(
      cluster.front,
      cluster.back,
      std::optional<int>(0),
      [&](auto v, auto acc) {
        if (v && acc) {
          return std::optional<int>(v.value() + acc.value());
        }
        else {
          return std::optional<int>();
        }
      },
      [&](auto v) {
        auto &task = graph[boost::vertex(v, graph)];
        return c.divided_cost(task);
      });
}

std::pair<Configurations::const_iterator, int> Clustering::opt_cluster_cost(
    const Cluster &cluster) const
{
  auto config = C.end();
  int  cost   = INT_MAX;
  for (auto it = C.begin(); it != C.end(); ++it) {
    auto c_cost = cluster_cost(*it, cluster);
    if (c_cost && c_cost.value() < cost) {
      cost   = c_cost.value();
      config = it;
    }
  }
  return std::make_pair(config, cost);
}

bool Clustering::merge()
{
  bool done = true;
  for (auto lhs = clusters.begin(), rhs = clusters.begin() + 1;
       rhs != clusters.end();
       ++lhs, ++rhs) {
    // TODO: consider case where empty clusters hinder merging
    //
    // This should simple skip rhs until there is one
    while(rhs != clusters.end() && rhs->is_empty()) {
      rhs++;
    }
    if (lhs->is_empty() || rhs->is_empty()) {
      continue;
    }
    auto combined      = lhs->expand(*rhs);
    auto combined_conf = opt_cluster_cost(combined);
    auto separate_cost = lhs->cost + rhs->cost + rho;
    if (combined_conf.second < separate_cost) {
      rhs->cost   = combined_conf.second;
      rhs->config = std::addressof(*combined_conf.first);
      rhs->front  = combined.front;
      rhs->back   = combined.back;
      lhs->empty();
      done = false;
    }
  }
  return done;
}

std::ostream &operator<<(std::ostream &os, const Cluster &c)
{
  if (!c.is_empty()) {
    os << *c.front << "-" << *c.back << " @ " << c.config->name << " -> "
       << c.cost;
  }
  else {
    os << "Empty cluster";
  }
  return os;
}
