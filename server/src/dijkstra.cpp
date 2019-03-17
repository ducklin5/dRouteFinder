#include <iostream>
#include "heap.h"
#include "heap.cpp"
#include "wdigraph.h"
#include "dijkstra.h"

using namespace std;

// create a struct of type edge with two parameters, beginning and end and define its constructor
struct Edge {
	Edge(int b, int e){
		begin = b;
		end = e;
	}
	int begin;
	int end;
#ifdef DEBUG
	friend ostream & operator << (ostream &out, const Edge &e);
#endif
};
	
#ifdef DEBUG
ostream & operator << (ostream &out, const Edge &e)
{
    out << e.begin << " --> " << e.end ;
    return out;
}
#endif
	
void dijkstra(const WDigraph& graph, int startVertex, unordered_map<int, PLI>& tree) {
	/*
	 * Compute least cost paths that start from a given vertex
	 * Use a binary heap to efficiently retrieve an unexplored
	 * vertex that has the minimum distance from the start vertex
	 * at every iteration
	 * PLI is an alias for "pair<long long, int>" type as discussed in class
	 * PARAMETERS:
	 * WDigraph: an instance of the weighted directed graph (WDigraph) class
	 * startVertex: a vertex in this graph which serves as the root of the search tree
	 * tree: a search tree used to construct the least cost path to some vertex
	 * */
	
	// instatiate a binary heap that will be used in the algorithm and insert the start vertex into the heap giving it an idex of 0 for reference
	BinaryHeap<Edge,long long> events;
	events.insert(Edge(startVertex,startVertex),0);


	// begin iterating through the set of vertices that are not yet in the shortest path set and
	// the vertex with the minimum cost to the shortest path set, then update the distance of the
	// neighboring vertices. Repeat until the least cost paths for every vertex is formed
	while(events.size() > 0){
		pair<Edge, long long> minEdgeCost = events.min();
		events.popMin();
		int u = minEdgeCost.first.begin;
		int v = minEdgeCost.first.end;
		int time = minEdgeCost.second;
		
		if(tree.find(v) == tree.end()){
#ifdef DEBUG
			cout << v << " was found in the tree\n";
#endif
			tree[v]={time,u};
			for (auto itW=graph.neighbours(v); itW != graph.endIterator(v); itW++ ){
				int w = *itW;
				Edge newEdge(v,w);
				events.insert(newEdge, time + graph.getCost(v,w));
			}
		}
#ifdef DEBUG
		events.draw(0,0);
#endif
	}
	
}
#ifdef DIJKSTRA_DBG
int main(){
	WDigraph test;
	test.addVertex(0);
	test.addVertex(1);
	test.addVertex(2);
	test.addVertex(3);
	test.addVertex(4);
	test.addVertex(5);
	test.addEdge(0,2,3);
	test.addEdge(1,2,3);
	test.addEdge(2,3,4);
	test.addEdge(3,4,5);
	test.addEdge(4,5,6);
	unordered_map<int, PLI> tree;
	dijkstra(test, 0, tree);
	return(0);
}
#endif
