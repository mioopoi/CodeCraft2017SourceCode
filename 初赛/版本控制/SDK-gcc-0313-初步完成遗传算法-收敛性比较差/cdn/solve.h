#ifndef __SOLVE_H__
#define __SOLVE_H__

#include <iostream>
#include <cstdio>
#include <vector>

#include "deploy.h"
#include "graph.h"
#include "globals.h"

#include "lib_record.h"
#include "lp_lib.h"

using namespace std;


class IntegerProgramming
{
public:
    vector<vector<int> > node_arc_matrix;  // 点弧关联矩阵
    vector<int> cost_vec;                  // 费用系数向量
    vector<int> demand_vec;                // 需求向量（为正表示服务节点, 为负表示消费节点）
    vector<int> capacity_vec;              // 边容量
    IntegerProgramming(Graph* graph);
    void construct_model(Graph* graph);
};

IntegerProgramming::IntegerProgramming(Graph* graph)
{
    this->node_arc_matrix = vector<vector<int> >(MAX_NODE_NUMBER, vector<int>(MAX_EDGE_NUMBER, 0));
    this->cost_vec = vector<int>(MAX_EDGE_NUMBER, 0);
    this->demand_vec = vector<int>(MAX_NODE_NUMBER, 0);
    this->capacity_vec = vector<int>(MAX_EDGE_NUMBER + MAX_NODE_NUMBER, 2000);

    int n = graph->net_node_num;  // 节点数量
    int m = graph->edge_num * 2;      // 有向边数量

    for (int i = 0; i < m; ++i)
    {
        Edge* edge = graph->net_edges[i];
        this->node_arc_matrix[edge->src][i] = 1;  // 边i是节点edge->src的出边
        this->node_arc_matrix[edge->des][i] = -1; // 边i是节点edge->des的入边
        this->cost_vec[i] = edge->fee;
        this->capacity_vec[i] = edge->cap;
    }
    // 将超源点编号为n，超源点的出边有n个，编号为m到(m+n-1)
    int node_no = 0;
    for (int i = m; i < m+n; ++i)
    {
        this->node_arc_matrix[node_no++][i] = -1;  // 边i是节点node_no的入边
        this->node_arc_matrix[n][i] = 1;           // 边i是超源点的出边
    }

    int sum_demand = 0;
    for (Cus_Node* demand_node : graph->cus_nodes)
    {
        this->demand_vec[demand_node->src] = -demand_node->need;
        sum_demand += demand_node->need;
    }
    this->demand_vec[n] = sum_demand;  // 超源点的供应量是所有消费节点需求的总和
}

void IntegerProgramming::construct_model(Graph* graph)
{
    lprec *lp;
    int ncol, nrow, *colno = NULL, ret = 0;
    REAL *row = NULL;

    ncol = graph->edge_num * 2 + graph->net_node_num;  // 决策变量的数量 = 边的数量 + 节点的数量（超源点连接了所有的节点）
    nrow = graph->net_node_num + 1;  // 最后一行是超源点流量约束

    lp = make_lp(0, ncol);  // 初始化模型
    if (lp == NULL)
    {
        ret = 1;
    }

    if (ret == 0)
    {
        colno = (int *) malloc(ncol * sizeof(*colno));
        row = (REAL *) malloc(ncol * sizeof(*row));
        if (colno == NULL || row == NULL)
        {
            ret = 2;
        }
    }

    if (ret == 0)
    {
        set_add_rowmode(lp, TRUE);
    }

    // 构造每个节点的供给/需求约束（每个节点：流出量之和 - 流入量之和 = 需求）
    for (int i = 0; ret == 0 && i < nrow; ++i)
    {
        // 第i个节点
        for (int j = 0; j < ncol; ++j)
        {
            colno[j] = j + 1;
            row[j] = this->node_arc_matrix[i][j];
        }
        if (!add_constraintex(lp, ncol, row, colno, EQ, demand_vec[i])) ret = 3;
    }

    // 构造边容量约束
    if (ret == 0)
    {
        for (int j = 0; j < ncol; ++j)
        {
            set_bounds(lp, j + 1, 0, this->capacity_vec[j]);
        }
    }

    // 所有决策变量为整数
    /*
    if (ret == 0)
    {
        for (int i = 1; i <= ncol; ++i)
        {
            set_int(lp, i, TRUE);
        }
    }*/

    // 构建目标函数
    if (ret == 0)
    {
        set_add_rowmode(lp, FALSE);

        for (int j = 0; j < ncol; ++j)
        {
            colno[j] = j + 1;
            row[j] = this->cost_vec[j];
        }
        if (!set_obj_fnex(lp, ncol, row, colno)) ret = 4;
    }
    if (ret == 0)
    {
        set_minim(lp);
        //write_LP(lp, stdout);
        set_verbose(lp, IMPORTANT);
        ret = solve(lp);
        if (ret == OPTIMAL) ret = 0;
        else ret = 5;

        get_variables(lp, row);
        for (int j = 0; j < ncol; ++j)
        {
        	cout << "C" << j+1 << ": " << row[j] << " ";
        }
        cout<<endl;
    }

    if (ret == 0)
    {
        printf("Objective value: %f\n", get_objective(lp));
        if (lp)
        {
            delete_lp(lp);
        }
        return;
    }
}


#endif
