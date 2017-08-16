#ifndef __GRAPH_H__
#define __GRAPH_H__

#include "globals.h"

#include <stdlib.h>
#include <string.h>

#include <vector>
#include <iostream>

using namespace std;

//边结构体定义
struct Edge
{
	int src;                   //边的源点
	int des;                   //边的目的点
	int cap;                   //边上的容量
	int fee;                   //单位容量的费用

	Edge(int src, int des, int cap, int fee)
	{
		this->src = src;
		this->des = des;
		this->cap = cap;
		this->fee = fee;
	}

	Edge() {}
};

//消费节点结构体定义
struct Cus_Node
{
	int src;                                        //消费节点编号
	int des;                                        //与消费节点相连的网络节点的编号
	int need;                                       //消费节点的需求

	Cus_Node(int src, int des, int need)
	{
		this->src = src;
		this->des = des;
		this->need = need;
	}

	Cus_Node() {}
};

//存放网络的拓扑数据
class Graph
{
public:
	vector<Edge*> net_edges;                         /* 存放网络中的所有有向边，下标即是边的编号，边的编号从0开始，
	                                                    数组大小为edge_num * 2， 0至edge_num-1放的是前向边, edge_num至2* edge_num - 1 放置的是反向边*/
	vector<vector<Edge*>> node_out_edges;            // 存放每个网络节点的出边，网络节点的编号从0开始
	vector<Cus_Node*> cus_nodes;                     // 存放所有消费者节点，消费节点的编号从0开始
	int net_node_num;                                // 网络节点的数量
	int cus_node_num;                                // 消费节点的数量
	int edge_num;                                    // 无向边的数量
	int net_fee;                                     // 服务器部署费用
	Graph(char* graph[MAX_EDGE_NUM], int line_num);  
	void extract_net_edges(char* graph[MAX_EDGE_NUM], int start);  //从字符串数据中解析出边数据
	void extract_cus_nodes(char* graph[MAX_EDGE_NUM], int start);  //从字符串数据中解析出消费节点数据
};

Graph::Graph(char* graph[MAX_EDGE_NUM], int line_num)
{
	char * delims = " ";
	char* result = strtok(graph[0], delims);
	this->net_node_num = atoi(result);
	this->edge_num = atoi(strtok(NULL, delims));
	this->cus_node_num = atoi(strtok(NULL, delims));

	this->net_fee = atoi(graph[2]);
	cout<<net_node_num<<" "<<edge_num<<" "<<cus_node_num<<" "<<net_fee<<endl;

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

		cur = new Edge(src, des, cap, fee);
		this->net_edges[cnt] = cur;                          //第cnt个位置放前向边
		this->node_out_edges[src][cnts[src]++] = cur;

		cur = new Edge(des, src, cap, fee);
		this->net_edges[cnt + this->edge_num] = cur;         //第cnt+edge_num个位置放对应的反向边
		this->node_out_edges[des][cnts[des]++] = cur;

		cnt++;
	}
	cout<<"total net edges: "<<cnt<<endl;
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
		this->node_out_edges[des][MAX_OUT_EDGE] = new Edge(des, src, 0, need); /* 把消费节点与网络节点相连的边记录在了node_out_edges[des]的最后一个位置上
		                                                                          不与消费节点连接的网络节点这个位置上放的是NULL*/
	}
	cout<<"total customer: "<<cnt<<endl;
};

#endif
