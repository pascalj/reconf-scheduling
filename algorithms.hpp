#pragma once

#include <stddef.h>

#include "scheduling.hpp"

extern int rho;

Schedule lsl(const Graph &g, const Configurations &C, size_t L);
Schedule cluster(Graph &g, Configurations &C);


struct Cluster {
  using order   = std::vector<Vertex>;
  using OrderIt = order::const_iterator;

  OrderIt              front;
  OrderIt              back;
  int                  cost   = 0;
  Configuration const *config = nullptr;

  explicit Cluster();
  explicit Cluster(OrderIt f, OrderIt b);
  explicit Cluster(OrderIt f, OrderIt b, const Configuration &c, int _cost);

  Cluster              expand(const Cluster &other) const;
  void                 empty();
  bool                 is_empty() const;
  friend std::ostream &operator<<(std::ostream &os, const Cluster &S);
};

struct Clustering {
  Graph               graph;
  std::vector<Vertex> order;
  std::vector<Cluster> clusters;
  Configurations       C;

  Clustering(Graph &&g, const Configurations &configs);

  std::optional<int> cluster_cost(
      const Configuration &, const Cluster &) const;
  std::pair<Configurations::const_iterator, int> opt_cluster_cost(
      const Cluster &) const;
  bool merge();
};
