#ifndef _DIJKSTRA_H_
#define _DIJKSTRA_H_

#include <unordered_map>
#include <utility> // for pair
#include "wdigraph.h"

typedef pair<long long, int> PLI;
void dijkstra(const WDigraph& graph, int startVertex, unordered_map<int, PLI>& tree);

#endif
