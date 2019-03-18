// ---------------------------------------------------
//    Name: Adekunle Toluwani & Azeez  Abass
//    ID's: 1538562 & 1542780
//    CMPUT 275 EB1, Winter 2019
//
//    Assignment 2: Driving Route Finder (Part 1)
//    File: Server.cpp
// ---------------------------------------------------

#include <iostream> // cout for debugging
#include <unordered_map> // unordered_map class
#include <vector> // vector class
#include <algorithm> // needed for sort function
#include "wdigraph.h" //  wdigraph class
#include "digraph.cpp" //  digraph class
#include <cstdlib> // abs function
#include <fstream> // input and output file streams
#include <string>  // string class
#include <sstream> // string spliting 
#include <stack>   // stack class
#include "dijkstra.h" // dijkstra algorithim function
#include "serialport.h" // Serial Communication

using namespace std;

struct Point {
	long long lat; // latitude of the point
	long long lon; // longitude of the point
};

long long manhattan(const Point& pt1, const Point& pt2) {
	// Return the Manhattan distance between the two given points
	long long latDiff = abs(pt1.lat - pt2.lat);
	long long lonDiff = abs(pt1.lon - pt2.lon);
	return latDiff + lonDiff;
}

void readGraph(string filename, WDigraph& graph, unordered_map<int, Point>& points) {
	/*
	 * Read the Edmonton map data from the provided file
	 * and load it into the given WDigraph object.
	 * Store vertex coordinates in Point struct and map
	 * each vertex to its corresponding Point struct.
	 * PARAMETERS:
	 * filename: name of the file describing a road network
	 * graph: an instance of the weighted directed graph (WDigraph) class
	 * points: a mapping between vertex identifiers and their coordinates
	 * */

	// Read each line of data from the map file into the variable line
	ifstream ifs;
	ifs.open (filename); // open the map file
	string line;
	while(getline(ifs, line)) // To get you each line
	{

		// split each line by the "," deliminator and save each sub string
		// in lineData
		string lineData[5];
		string token;
		stringstream ss(line);
		unsigned int x = 0;
		while(getline(ss, token, ',')) {
			lineData[x] = token;
			x++;
		}


		// if the command is V, add a vector 
		if (lineData[0]=="V"){
			// convert the 1st element to ID
			unsigned int ID = stoi(lineData[1]);
			// 2nd and 3rd elements to lat and lon
			long long lat = static_cast<long long> (stold(lineData[2])*100000);
			long long lon = static_cast<long long> (stold(lineData[3])*100000);
			// add lat and lon to a new point instance
			Point p = {lat, lon};
			// add the id to a graph object
			graph.addVertex(ID);
			// add the point to the unordered_map (points)
			points.insert({ID, p});

			// otehrwise if the command E 
			// add an edge (directional)
		} else if (lineData[0]=="E"){
			// get the 1st and 2nd subStrings, save them as ints
			unsigned int ID1 = stoi(lineData[1]);
			unsigned int ID2 = stoi(lineData[2]);
			// the corresponding points for the IDs
			Point pt1 = points[ID1];
			Point pt2 = points[ID2];
			// Add the edge with its cost (manhattan distance between the points)
			graph.addEdge(ID1,ID2, manhattan(pt1, pt2));
		}
	}
	// close the input file
	ifs.close();
}
#ifdef DEBUG
void printTree (unordered_map<int,PLI>& tree){
	/**
	 * Prints an unordered <int, PLI> map
	 * (made for dijkastra output tree)
	 **/
	for (auto edge: tree){
		int stop = edge.first;
		int start = edge.second.second;
		int cost = edge.second.first;
		cout << stop << " :: " << cost << "," << start << endl;
	}
}
#endif

pair<Point, Point> getRequest( ifstream &ifs){
	// *********************
	// Wait for command input then get start and end points
	// *********************
	pair<Point, Point> request;
	string inputline; // where line will be stored
	string lineData[6]; // where line substring will be stored

	// wait for input from the file/ arduino
	do {
		// get a line and check for eof
		if(!getline(ifs, inputline)){
			throw -1;
			return request;
		}
		// split the line into substrings
		// split each line by the " " deliminator and save each sub string
		string token;
		stringstream ss(inputline);
		unsigned int x = 0;
		while(getline(ss, token, ' ')) {
			lineData[x] = token;
			x++;
		}
		// reapeat till the first R command is found 
	} while (lineData[0] != "R"); 

	// get the start and end point of the requested path
	Point startP = { stoll(lineData[1]), stoll(lineData[2])};
	Point endP = { stoll(lineData[3]), stoll(lineData[4])};

	request = {startP, endP};
	return request;
}

pair<int, int> getStartEndID(unordered_map<int, Point> coordTable, Point startP, Point endP){
	// *******************************
	// Find The closest point IDs in the database to the requested points
	// *******************************
	// calculate the distance from all the points in the map to the start and end points
	// vector of (long long, int) pairs
	vector<PLI> distFromStart;
	vector<PLI> distFromEnd;
	for(auto point : coordTable){
		// for each point in the coordinates Table
		// get its ID
		int ID = point.first;

		// calculate its distance from the start and end points
		long long distS = manhattan(startP, point.second);
		long long distE = manhattan(endP, point.second);

		// add each distance to the appropriate vector along with the point ID
		distFromStart.push_back(make_pair(distS,ID));
		distFromEnd.push_back(make_pair(distE,ID));
	}

	// sort each list by the first item in the pair (distance)
	sort(distFromStart.begin(),distFromStart.end());
	sort(distFromEnd.begin(),distFromEnd.end());

	// get the closest point to the requested start and end points
	int startID = distFromStart[0].second;
	int endID = distFromEnd[0].second;

#ifdef DEBUG
	cout << "Nearest dist from start is:\n" ; 
	cout << distFromStart[0].first << " "
		<< startID << endl; 
	cout << "Nearest dist from end is:\n" ; 
	cout << distFromEnd[0].first << " "
		<< endID << endl;
#endif

	return make_pair(startID, endID);
}

stack<int> getReversePath ( WDigraph& graph, int startID, int endID){
	// *******************************
	// Find the shorstest path from start to end points
	// *******************************

	// find the shortest path from the start point to any accesible point
	// eliminate unnecesary edges from the graph
	// use an unorderd map to store the new edge tree
	unordered_map<int, PLI> edgeTree;
	dijkstra( graph, startID, edgeTree );

#ifdef DEBUG
	printTree(edgeTree);
#endif

	// traverse the tree backward (from the end point to start point)
	// adding each waypoint ID to a stack (reversePath)
	stack<int> reversePath; // to hold points in reverse order
	int current = endID; //store the current point

	// if dijkstra was able access the end point from the start point
	if ( edgeTree.find(endID) != edgeTree.end() ){
		// add the endPoint to the stack
		reversePath.push(current);
		// keep adding the parent node till the start node is added
		while (current != startID){
			current = edgeTree.at(current).second;
			reversePath.push(current);
		}	
	}
	// revesePath stack now contains the path from the start to end nodes
	return reversePath;
}

void printPath( unordered_map<int, Point> coordTable, stack<int> reversePath, SerialPort &ios){
	// *******************************
	// Write the path in reversePath stack to the output file
	// *******************************
	// write the number of waypoints in the path
	

	ios.writeline("N ");
	ios.writeline(to_string(reversePath.size()));
	ios.writeline("\n");
	
	string line;
	line = ios.readline();
	cout << line;
	// print the point lat and lon for each waypoint in the stack
	// start point to end point
	//

	string inputline; // where line will be stored
	while (!reversePath.empty()) { 
		int ID = reversePath.top();
		Point current = coordTable.at(ID);

		ios.writeline("W "); 
		ios.writeline(to_string(current.lat));
		ios.writeline(" ");
		ios.writeline(to_string(current.lon));
		ios.writeline("\n");

		line = ios.readline();
		cout << line;

		reversePath.pop();
		// wait for the acknolegment before proceeding
		while (inputline != "A"){
			getline(ifs, inputline);
		}
	}
	ios.writeline("E\n"); 
	
	line = ios.readline();
	cout << line;
}

int main(int argc, char* argv[]){
	// *******************************
	// Read server input, Set up the server 
	// *******************************

	// Make sure there are wnough arguments
	if (argc > 2){
		cout << "Warning: Excess arguments, remaining Arguments will be ignored.\n";
	} else if ( argc < 2){
		cout << "Error: Insufficient arguments! Please read REAME\n"; return -1;
	}

	// get the input command filename
	char* inputCFile = argv[1];

	// get the graph of the city
	WDigraph graph;
	unordered_map<int, Point> coordTable;
	readGraph("edmonton-roads-2.0.1.txt", graph, coordTable);

	// open the input command file for reading
	ifstream ifs;
	ifs.open (inputCFile);


	// get the start and end points from the client
	pair<Point, Point> request; 
	try {
		request = getRequest(ifs);
	} catch (int error) {
		cout << "No R command in file before end of file, or file could not be read!\n";
		return error;
	}
	Point startP = request.first;
	Point endP = request.second;

	// get the corresponding IDs
	pair<int, int> ids = getStartEndID(coordTable, startP, endP);

	int startID = ids.first;
	int endID = ids.second;

	// get the path from the IDs
	stack <int> reversePath = getReversePath(graph, startID, endID);

	// open the output file for writting

	SerialPort Serial("/dev/ttyACM0");
	printPath( coordTable, reversePath, ifs, Serial);

	return 0;
}

