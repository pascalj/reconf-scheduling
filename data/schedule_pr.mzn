int: ntasks;
int: nprocs;
% Configuration Overhead
int: rho;

% The different configurations
enum C;
% All tasks
set of int: V = 1..ntasks;
% Processing elements across all configurations
set of int: P = 1..nprocs;

% Mapping of processing elements to their config
array[P] of C: P_config;
% Cost of running task V on processing element P
array[V,P] of opt int: cost;
% Dependency matrix
array[V,V] of bool: deps;

% Start times of tasks
array[V] of var int: t_s;
% Finish times of tasks
array[V] of var int: t_f;
% PE a task runs on
array[V] of var int: process;

array[V] of string: tasklabels;

predicate no_overlap(var int: v1, var int: v2) =
        t_f[v1] < t_s[v2] \/ t_s[v1] > t_f[v2];
predicate overlap(var int: v1, var int: v2) = not no_overlap(v1, v2);

% Set the finish time depending on the cost
constraint forall(v in V)(t_f[v] = t_s[v] + deopt(cost[v,process[v]]));
constraint forall(v in V)(if occurs(cost[v,process[v]]) then deopt(cost[v,process[v]]) > 0 else true endif);

% Start schedule at 0
constraint forall(v in V)(t_s[v] >= rho + 1);

% If task v1 and v2 execute on the same process, they must not overlap
constraint forall(v1, v2 in V where v1 < v2)(
        if process[v1] == process[v2] then
                t_f[v1] < t_s[v2] \/ t_s[v1] > t_f[v2] else true endif);

% If task v1 and v2 overlap, they must use the same configuration
/* constraint forall(v1, v2 in V where v1 < v2)( */
/*         if overlap(v1, v2) then */
/*                 P_config[process[v1]] == P_config[process[v2]] */
/*                 else true endif); */

% All dependecies must be fullfilled
constraint forall(v1, v2 in V)(if deps[v2,v1] then t_f[v1] < t_s[v2] else true endif);

% Each task runs on exactly one PE
constraint forall(v in V)(process[v] > 0 /\ process[v] <= nprocs);
constraint forall(v in V, p in P)(if absent(cost[v,p]) then process[v] != p else true endif);


% A reconfiguration blocks all tasks for rho
constraint forall(v1, v2 in V)(
        if t_f[v1] < t_s[v2] /\ process[v1] == process[v2] /\ P_config[process[v1]] != P_config[process[v2]] then
                t_s[v2] > t_f[v1] + rho
        else true endif);

solve :: int_search(process, first_fail, indomain_random) :: restart_linear(1000) minimize max(v in V)(t_f[v]);
/* solve satisfy; */

output[
        tasklabels[i] ++
        /* show_int(2, i) ++ ": " ++ */
        show(fix(t_s[i])) ++ " " ++ show(fix(t_f[i])) ++
        concat([" " | a in 1..fix(t_s[i])]) ++
        concat([show(process[i]) | a in 1..fix(t_f[i] - t_s[i])]) ++
        "\n" | i in index_set(V)];
output[
        "minizinc," ++ show(ntasks) ++ "," ++ show(max(v in V)(t_f[v]))];


