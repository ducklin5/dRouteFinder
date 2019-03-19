#include "heap.h"
#include <iostream>

using namespace std;

#ifdef DEBUG
template <class T, class K>
void BinaryHeap<T,K>::draw(int index, int tabs) const {
	for(int i = 0; i < tabs; i++){ cout << "\t↳ ";}
	cout << "(" << heap[index].first << ", " << heap[index].second << ")\n";

	unsigned int left = 2*index+1;
	unsigned int right = 2*index+2;

	if(left < heap.size()){
		draw(left, tabs+1);
	}
	if(right < heap.size()){
		draw(right, tabs+1);
	}
}
#endif

// method to return the root of the heap tree
template <class T, class K>
pair<T, K> BinaryHeap<T,K>::min() const {
	return heap[0];
}

// method to insert a new element into the node of the heap tree. 
// After inserting into the very last node, reorder the tree using fixUp method. 
template <class T, class K>
void BinaryHeap<T,K>::insert(const T& item, const K& key){
	pair<T, K> v (item, key);
	int i = heap.size();
	heap.insert(heap.end(),v);
#ifdef HEAP_DBG
	draw(0,0);
#endif
	fixUp(i);
}

// fixUP method works by percolating up i.e. comparing the added element with 
// the parent and moving the added element up a level
template <class T, class K>
int BinaryHeap<T,K>::fixUp(int index){
	pair<T, K> v = heap[index];
	pair<T, K> vParent = heap[(int) (index-1)/2];
	int finalIndex = index;
	if(index != 0 and v.second < vParent.second){
		// swap
		heap[(int) (index-1)/2] = v;
		heap[ index ] = vParent;
#ifdef HEAP_DBG
		draw(0,0);
#endif
		// update index
		index = (int) (index-1)/2;
		finalIndex = fixUp(index);
	}
	return finalIndex;
}


// method to remove the minimum element in the heap (the root). After removing the root
// replace with the last element in the heap and reorder the heap by using fixDown method
template <class T, class K>
void BinaryHeap<T,K>::popMin(){
	pair<T, K> last = heap.back();
	heap[0] = last;
	heap.pop_back();
#ifdef HEAP_DBG
	draw(0,0);
#endif
	fixDown(0);	
}


// fixDown method works by percolating down i.e.comparing the current root (which is also the last element)
// with the children nodes and moving the element down a level
template <class T, class K>
int BinaryHeap<T,K>::fixDown(int index){
	pair<T, K> v = heap[index];
	unsigned int left = 2*index+1;
	unsigned int right = 2*index+2;

	int finalIndex = index;
	if ( left < heap.size() ){
		unsigned int lkIndex = left;
		if (right < heap.size()){
			if(heap[right].second < heap[left].second){
				lkIndex = right;
			}
		}
		pair<T, K> lkChild = heap[lkIndex];
		heap[index] = lkChild;
		heap[lkIndex] = v;
#ifdef HEAP_DBG
		draw(0,0);
#endif
		finalIndex = fixDown(lkIndex);
	}
	return finalIndex;
}	


// method that returns the size of the heap tree
template <class T, class K>
int BinaryHeap<T,K>::size() const{
	return heap.size();
}
#ifdef HEAP_DBG
int main(){
	BinaryHeap<int,int> test;
	test.insert(-19,9);
	test.insert(0,4);
	test.insert(13,2);
	test.insert(12,0);
	test.popMin();
	cout << test.min().first << ", " << test.min().second << "\n";

}
#endif
