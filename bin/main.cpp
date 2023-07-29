#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <vector>

#include "lib/allocator.h"

using namespace std;
using namespace mtl;


int main() {
    PoolAllocator<int> p(10, 1000);
    int* arr = p.allocate(200);
    arr[199] = 241;
    cout << arr[199];
    return 0;
}
