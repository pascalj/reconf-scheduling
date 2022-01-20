#pragma once

#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include "json.hpp"
#include "scheduling.hpp"
#include "simple_svg.hpp"

extern int rho;

Graph          import_task_graph(const nlohmann::json &);
Configurations import_configs(const nlohmann::json &);
void           export_svg(const Schedule &, const std::string &);
