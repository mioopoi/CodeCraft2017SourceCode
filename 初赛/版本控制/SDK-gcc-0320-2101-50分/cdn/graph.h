#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "globals.h"

#include <stdlib.h>
#include <string.h>

#include <vector>
#include <iostream>

using namespace std;


struct Edge
{
	int src;
	int des;
	int cap;
	int fee;

	Edge(int src, int des, int cap, int fee) : src(src), des(des), cap(cap), fee(fee) {}

	Edge() {}

    void* operator new(size_t, void* p)    {return p;}
};

struct Cus_Node
{
	int src;
	int des;
	int need;

	Cus_Node(int src, int des, int need)
	{
		this->src = src;
		this->des = des;
		this->need = need;
	}

	Cus_Node() {}
};

class Graph
{
public:
    Edge* edges_storage;
    Edge* edges_storage_copy;
	vector<Edge*> net_edges;                         // all directed edges between net_node
	vector<vector<Edge*>> node_out_edges;            // each net_node's out edges
	vector<Cus_Node*> cus_nodes;                     // all customer node
	int net_node_num;                                // network node number
	int cus_node_num;                                // customer node number
	int edge_num;                                    // undirected edge num
	int net_fee;                                     // place server fee
	Graph(char* graph[MAX_EDGE_NUM], int line_num);
	~Graph();
	void extract_net_edges(char* graph[MAX_EDGE_NUM], int start);
	void extract_cus_nodes(char* graph[MAX_EDGE_NUM], int start);
};

Graph::Graph(char* graph[MAX_EDGE_NUM], int line_num)
{
	char * delims = " ";
	char* result = strtok(graph[0], delims);
	this->net_node_num = atoi(result);
	this->edge_num = atoi(strtok(NULL, delims));
	this->cus_node_num = atoi(strtok(NULL, delims));

	this->net_fee = atoi(graph[2]);
	//cout<<net_node_num<<" "<<edge_num<<" "<<cus_node_num<<" "<<net_fee<<endl;

    this->edges_storage = new Edge[this->edge_num * 2 + this->cus_node_num];
    this->edges_storage_copy = this->edges_storage;

	extract_net_edges(graph, 4);
	extract_cus_nodes(graph, 5 + this->edge_num);
};

void Graph::extract_net_edges(char* graph[MAX_EDGE_NUM], int start)
{
	this->net_edges = vector<Edge*>(this->edge_num*2);
	this->node_out_edges = vector<vector<Edge*>>(this->net_node_num, vector<Edge*>(MAX_OUT_EDGE + 1, NULL));
	vector<int> cnts = vector<int>(this->net_node_num, 0);
	char* delims;
	char* result;
	int src, des, cap, fee, end = start + this->edge_num, cnt = 0;
	Edge* cur;
	for(int i = start; i < end; i++)
	{
		delims = " ";
		result = strtok(graph[i], delims);
		src = atoi(result);
		des = atoi(strtok(NULL, delims));
		cap = atoi(strtok(NULL, delims));
		fee = atoi(strtok(NULL, delims));

		cur = new (this->edges_storage++) Edge(src, des, cap, fee);
		this->net_edges[cnt] = cur;
		this->node_out_edges[src][cnts[src]++] = cur;

		cur = new (this->edges_storage++) Edge(des, src, cap, fee);
		this->net_edges[cnt + this->edge_num] = cur;
		this->node_out_edges[des][cnts[des]++] = cur;

		cnt++;
	}
	//cout<<"total net edges: "<<cnt<<endl;
};

void Graph::extract_cus_nodes(char* graph[MAX_EDGE_NUM], int start)
{
	this->cus_nodes = vector<Cus_Node*>(this->cus_node_num);
	char* delims;
	char* result;
	int src, des, need, end = start + this->cus_node_num, cnt = 0;
	for(int i = start; i < end; i++)
	{
		delims = " ";
		result = strtok(graph[i], delims);
		src = atoi(result);
		des = atoi(strtok(NULL, delims));
		need = atoi(strtok(NULL, delims));
		this->cus_nodes[cnt++] = new Cus_Node(src, des, need);
		this->node_out_edges[des][MAX_OUT_EDGE] = new (this->edges_storage++) Edge(des, src, 0, need); //last position place the edge with custoner
	}
	//cout<<"total customer: "<<cnt<<endl;
};

Graph::~Graph()
{
    delete this->edges_storage_copy;
    for(int i = 0; i < this->cus_node_num; i++)
    {
        delete cus_nodes[i];
    }
}

#endif
