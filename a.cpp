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
    cout << sizeof(".word") << endl;
    vector<string> output{"aaa", "bbb"};
    for (size_t i = 0; i < output.size(); i++)
    {
        string s = output[i];
        if (s.size() > 1)
        {
            while (s.size() > 1)
            {
                output.insert(output.begin() + i, s.substr(0, 1));
                ++i;
                s = s.substr(1, string::npos);
                if (s.size() <= 1)
                    output[i] = s;
            }
        }
        while (output[i].size() < 1)
            output[i].push_back('0');
    }
    for (string &s : output)
        cout << s << endl;
    return 0;
}