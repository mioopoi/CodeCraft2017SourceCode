#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "lib/lib_io.h"

using namespace std;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-variable"

const int MAX_EDGE_NUMBER = 110010;
const int MAX_NODE_NUMBER = 10010;
const int INF = 100000000;

int g_edge_src[MAX_EDGE_NUMBER];
int g_edge_des[MAX_EDGE_NUMBER];
int g_edge_cap[MAX_EDGE_NUMBER];
int g_edge_cost[MAX_EDGE_NUMBER];
int g_edge_next[MAX_EDGE_NUMBER];

int g_node_head[MAX_NODE_NUMBER];
int g_node_cost[MAX_NODE_NUMBER];

int g_node_customer[MAX_NODE_NUMBER];
int g_customer_need[MAX_NODE_NUMBER];

int g_server_level_index[10001];

struct ServerLevel
{
    int level;
    int power;
    int cost;

    ServerLevel(int level = -1, int power = -1, int cost = -1) : level(level), power(power), cost(cost) {}
};

ServerLevel g_server_level[10];

class Graph
{
public:
    int node_num;
    int customer_num;
    int edge_num;
    int level_num;
    int cur_line;
    Graph(char * graph[MAX_EDGE_NUM], int line_num);
    void extract_server_level(char* graph[MAX_EDGE_NUM]);
    void extract_node_cost(char* graph[MAX_EDGE_NUM]);
    void extract_edges(char* graph[MAX_EDGE_NUM]);
    void extract_customers(char* graph[MAX_EDGE_NUM]);

};

Graph::Graph(char* graph[MAX_EDGE_NUM], int line_num)
{
    cur_line = 0;
    level_num = 0;
    char * delims = " ";
	char* result = strtok(graph[cur_line++], delims);
	this->node_num = atoi(result);
	this->edge_num = atoi(strtok(NULL, delims));
	this->customer_num = atoi(strtok(NULL, delims));

	cout<<node_num<<" "<<edge_num<<" "<<customer_num<<" "<<endl;
	extract_server_level(graph);
	extract_node_cost(graph);
	extract_edges(graph);
	extract_customers(graph);

}

void Graph::extract_server_level(char* graph[MAX_EDGE_NUM])
{
    cur_line++;

    char* delims, *result;
    int level, cnt = 1;
    g_server_level_index[0] = -1;
    while(graph[cur_line][0] != '\r')
    {
        delims = " ";
        result = strtok(graph[cur_line], delims);
        level = atoi(result);
        g_server_level[level].level = level;
        g_server_level[level].power = atoi(strtok(NULL, delims));
        g_server_level[level].cost = atoi(strtok(NULL, delims));
        level_num++;
        cur_line++;
        while(cnt <= g_server_level[level].power)
            g_server_level_index[cnt++] = level;
        //cout<<level<<" "<<g_server_level[level].power<<" "<<g_server_level[level].cost<<endl;
    }

    /*for(int i = 0; i <= g_server_level[level_num - 1].power; i++)
    {
        cout<<i<<" "<<g_server_level_index[i]<<endl;
    }*/
}

void Graph::extract_node_cost(char* graph[MAX_EDGE_NUM])
{
    cur_line++;
    char* delims, *result;
    int cost;
    for(int i = 0; i < node_num; i++)
    {
        delims = " ";
        result = strtok(graph[cur_line], delims);
        cost = atoi(strtok(NULL, delims));
        g_node_cost[atoi(result)] = cost;
        //cout<<result<<" "<<cost<<endl;
        cur_line++;
    }
}

void Graph::extract_edges(char* graph[MAX_EDGE_NUM])
{
    for(int i = 0; i < node_num; i++)
        g_node_head[i] = -1;

    cur_line++;
    int tol = 0, src, des, cap, cost;
    char* delims, *result;
    for(int i = 0; i < edge_num; i++)
    {
        delims = " ";
        result = strtok(graph[cur_line], delims);
		src = atoi(result);
		des = atoi(strtok(NULL, delims));
		cap = atoi(strtok(NULL, delims));
		cost = atoi(strtok(NULL, delims));

        g_edge_src[tol] = src;
        g_edge_des[tol] = des;
        g_edge_cap[tol] = cap;
        g_edge_cost[tol] = cost;
        g_edge_next[tol] = g_node_head[src];
        g_node_head[src] = tol++;

        g_edge_src[tol] = des;
        g_edge_des[tol] = src;
        g_edge_cap[tol] = cap;
        g_edge_cost[tol] = cost;
        g_edge_next[tol] = g_node_head[des];
        g_node_head[des] = tol++;
		cur_line++;

		//if(i> edge_num - 10)
            //cout<<src<<" "<<des<<" "<<cap<<" "<<cost<<endl;
    }
}

void Graph::extract_customers(char* graph[MAX_EDGE_NUM])
{
    for(int i = 0; i < node_num; i++)
        g_node_customer[i] = -1;
    cur_line++;
    char* delims, *result;
    int node, customer, need;
    for(int i = 0; i < customer_num; i++)
    {
        delims = " ";
        result = strtok(graph[cur_line], delims);
		customer = atoi(result);
		node = atoi(strtok(NULL, delims));
		need = atoi(strtok(NULL, delims));
		g_customer_need[customer] = need;
		g_node_customer[node] = customer;
		//cout<<node<<" "<<customer<<" "<<need<<endl;
		cur_line++;
    }
}

#endif
