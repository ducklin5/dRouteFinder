MAKEFLAGS += --no-builtin-rules
#--BUild Flags--------------------------------------
CXX=g++
CXXFLAGS=-Wall -std=c++11
#--Directory Variables------------------------------
SRC_DIR=src
OBJ_DIR=obj
dir_guard=@mkdir -p #make sure specified directory exists

objects = heap.o dijkstra.o digraph.o serialport.o server.o
EXE = server heapDBG dijkstraDBG
#--Targets------------------------------------------
# server: CXXFLAGS += -D DEBUG
server: $(objects)
	$(CXX) $(CXXFLAGS) obj/heap.o obj/dijkstra.o obj/server.o obj/serialport.o -o $@ 


heapDBG: CXXFLAGS += -D HEAP_DBG -D DEBUG
heapDBG: $(objects)
	$(CXX) $(CXXFLAGS) obj/heap.o -o $@ 

dijkstraDBG: CXXFLAGS += -D DIJKSTRA_DBG -D DEBUG
dijkstraDBG:  $(objects)
	$(CXX) $(CXXFLAGS) obj/heap.o obj/digraph.o obj/dijkstra.o -o $@ 


#target to build anytype of cpp file to an obj in the Obj dir
$(objects): %.o: $(OBJ_DIR)/%.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(dir_guard) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean: 
	rm -rf $(OBJ_DIR) $(EXE)

#---DEBUG-------------------------------------------
debug: CXXFLAGS += -D DEBUG
debug: clean
	$(info Cleaned and added Debug flag)
