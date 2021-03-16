#include <bitset>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <fstream>
using namespace std;

int main()
{
    unordered_map<int, int> m;
    m.emplace(1, 1);
    m.emplace(1, 2);
    for (auto &it : m)
        cout << it.first << ":" << it.second << endl;
    return 0;
}