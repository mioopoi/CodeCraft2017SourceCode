#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED

#include <string>
#include <vector>

using namespace std;

void construct_output(vector<stack<int> >& paths, char* &buffer, vector<bool> servers = vector<bool>(), int min_cost = 0)
{
    // 将paths序列化并写入buffer
    string ret = "";
    if(servers.size() != 0)
    {
        for(int i = 0; i < servers.size(); i++)
        {
            if(servers[i])
                ret = ret + to_string(i) + " ";
        }
        ret = ret + "\n\n" + to_string(min_cost) + "\n\n";
    }

    ret += to_string(paths.size());
    ret += "\n\n";
    for (int i = 0; i < paths.size(); ++i)
    {
        stack<int> &path = paths[i];
        while (!path.empty())
        {
            ret += to_string(path.top()); path.pop();
            if (!path.empty())
                ret += " ";
            else
                ret += "\n";
        }
    }
    ret.pop_back();
    //cout << ret << endl;
    buffer = new char[ret.length() + 1];
    strcpy(buffer, ret.c_str());
    //for (int i = 0; buffer[i]; ++i) printf("%c", buffer[i]);
}

#endif // OUTPUT_H_INCLUDED
