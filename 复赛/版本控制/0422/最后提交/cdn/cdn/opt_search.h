#ifndef __OPT_SEARCH_H__
#define __OPT_SEARCH_H__

#include <iostream>
#include <vector>
#include <algorithm>

#include "opt_search_helper.h"
#include "graph.h"
#include "net_simplex_mcf.h"

#define MAX_LEVEL_FLOW_COST 0
#define SERVER_LEVEL_FLOW_COST 1
#define ONLY_RUN 2
#define NO_UPDATE_LEVEL 3

int  g_neighbor_succeed[2000][2000];

int g_edge_src_cp[MAX_EDGE_NUMBER];
int g_edge_des_cp[MAX_EDGE_NUMBER];
int g_edge_cap_cp[MAX_EDGE_NUMBER];
int g_edge_cost_cp[MAX_EDGE_NUMBER];
int g_edge_next_cp[MAX_EDGE_NUMBER];
int g_node_head_cp[MAX_NODE_NUMBER];
int edge_num_cp;

int opt_cost_matrix[2000][2000];
int opt_flow_matrix[2000][2000];

class OptSearch
{
public:
    Graph* graph;
    NetSimplexMCF* mcf_simplex;
    int node_num;
    int customer_num;
    int level_num;
    int level_num_cp;

    vector<Candidate*> candidates;
    vector<Candidate*> candidates_cp;
    Candidate* candidate_start_ptr;

    int global_min_cost;
    int* servers;
    int* pre_servers;
    Candidate* empty_candidate;
    int topo_lv;
    vector<vector<ServerCost>> supply_costs;

public:
    OptSearch(Graph* graph);
    void init_candidates();
    bool show_detail_info();
    int mcf_cnt;
    clock_t start_time;
    clock_t end_time;
    bool time_out;
    int greedy_level;

public:
    void init_supply_costs();
    void calculate_candidates_neighbor();
    void calculate_candidate_out_flow();
    bool calculate_candidates_pass_cost();
    bool calculate_candidates_pass_flow();
    bool calculate_server_cost(vector<ServerCost>& server_costs);
    void calculate_candidates_overflow_cost();
    void count_used_edges();

    int choose_server_by_node_demand(double percent);
    int choose_server_by_node_max_flow(bool re_cal, double percent);
    void choose_server_by_pass_flow(bool re_cal, double percent);

    void construct_first_solution_combine_demand_and_flow();
    void construct_first_solution_by_min_server_cost();

public:
    bool run_simplex_mcf(int run_type);
public:
    void deal_with_topo_lv0_zero();
    void deal_with_topo_lv0_one();
    void deal_with_topo_lv1_zero();
    void lv0_add_server(int type);
    void topo_strategy();
    void deal_with_topo_lv1_one();
    void zdd_search();
    void punish_strategy();
    void deal_with_topo_lv0_two();
    void deal_with_topo_lv1_two();

    void show_supply_detail();
public:
    bool decrease_server_by_server_outflow_max_level();
    bool decrease_server_by_server_out_flow_no_adjust_level();
    bool decrease_server_by_server_outflow();

    bool increase_server_by_node_pass_cost_max_level();
    bool increase_server_by_node_pass_cost_no_adjust_level();
    bool increase_server_by_node_pass_cost();

    int exchange_node_no_adjust_level(int in_id, int in_lv, int out_id);
    bool exchange_server_by_server_cost_no_adjust_level();
    bool exchange_server_with_neighbor_no_adjust_level();
    bool exchange_server_with_neighbor_max_level();
    bool exchange_server_by_node_pass_cost_no_adjust_level();
    bool exchange_one_server_with_other_candidates_no_adjust_level(int out_id);
    bool exchange_server_by_node_pass_cost();
    bool exchange_one_node_with_all_server(int in_id);
    int  exchange_node(int in_id, int in_lv, int out_id, int& real_lv);
    int  exchange_node_max_level(int in_id, int in_lv, int out_id);
    bool exchange_server_by_server_cost();
    bool exchange_server_with_neighbor();
    bool exchange_server_by_node_pass_cost_lv0();
    bool exchange_one_server_with_other_candidates(int out_id);
    bool exchange_server_by_node_pass_cost_max_level();
    bool exchange_one_server_with_other_candidates_max_level(int out_id);

    bool zdd_down_server_level();
    bool chj_down_server_level();
    bool zdd_up_server_level();
    bool chj_up_server_level();
    bool chj_down_server_level_one();

public:
    bool construct_first_solution_by_simplex_graph();
    void save_original_graph();
    void construct_simplex_graph();
    void construct_simplex_graph_one();
    void recover_original_graph();
};

OptSearch::OptSearch(Graph* graph)
{
    start_time = clock();

    this->graph = graph;
    graph->edge_num = graph->edge_num * 2;
    level_num_cp = graph->level_num;

    mcf_simplex = new NetSimplexMCF(graph);
    node_num = graph->node_num;
    customer_num =graph->customer_num;
    level_num = graph->level_num;

    servers = new int[node_num];
    pre_servers = new int[node_num];

    memset(servers, -1, sizeof(int) * node_num);

    global_min_cost = INF;
    init_candidates();

    mcf_cnt = 0;
    start_time = clock();
    time_out = false;

    for(int i = 0; i < node_num; i++)
    {
        memset(g_neighbor_succeed[i], -1, sizeof(int) * node_num);
    }
    greedy_level = 0;

    if(node_num <= 600) topo_lv = 0;
    else topo_lv = 1;

    if(topo_lv == 0) graph->level_num = graph->level_num;
    else graph->level_num = graph->level_num - 1;
    //graph->level_num = graph->level_num / 2;
}

void OptSearch::init_candidates()
{
    int candidate_ptr_cnt = 0;
    candidate_start_ptr = new Candidate[node_num + 2];

    empty_candidate = new (candidate_start_ptr + candidate_ptr_cnt) Candidate();
    candidate_ptr_cnt++;

    candidates = vector<Candidate*>(node_num, NULL);
    candidates_cp = vector<Candidate*>(node_num, NULL);

    bool is_customer;
    int demand;
    for(int i = 0; i < node_num; i++)
    {
        if(g_node_customer[i] != -1)
        {
            is_customer = true;
            demand = g_customer_need[g_node_customer[i]];
        }
        else
        {
            is_customer = false;
            demand = 0;
        }
        candidates[i] = new (candidate_start_ptr + candidate_ptr_cnt) Candidate(i, demand, g_node_cost[i]);
        candidates_cp[i] = candidates[i];
        candidate_ptr_cnt++;
    }
}

void OptSearch::init_supply_costs()
{
    supply_costs = vector<vector<ServerCost>>(node_num, vector<ServerCost>(0));
    for(int i = 0; i < node_num; i++)
    {
        for(int j = 0; j < node_num; j++)
        {
            if(simplex_cost_matrix_cp[i][j] != INF)
            {
                supply_costs[j].push_back(ServerCost(i, j, simplex_cost_matrix_cp[i][j]));
                //cout<<i<<" "<<j<<" "<<simplex_cost_matrix_cp[i][j]<<endl;
            }
        }
    }

    for(int i = 0; i < node_num; i++)
    {
        sort(supply_costs[i].begin(), supply_costs[i].end(), server_cost_cmp_less());
    }

}

void OptSearch::calculate_candidates_neighbor()
{
    int max_flow, cus_num, des;
    for(int i = 0; i < node_num; i++)
    {
        max_flow = 0, cus_num = 0;
        if(g_node_customer[i] != -1)
            cus_num++;
        for(int j = g_node_head[i]; j != -1; j = g_edge_next[j])
        {
            max_flow += g_edge_cap[j];
            des = g_edge_des[j];
            if(g_node_customer[des] != -1)
                cus_num++;
        }
        candidates_cp[i]->max_flow = max_flow;
        candidates_cp[i]->cus_num = cus_num;
        //cout<<i<<" "<<max_flow<<" "<<cus_num<<endl;
    }
}

void OptSearch::calculate_candidate_out_flow()
{
    for(int i = 0; i < node_num; i++)
    {
        candidates_cp[i]->server_out_flow = 0;
    }
    run_simplex_mcf(2);
    mcf_simplex->calculate_server_out_flow(candidates_cp);
    //zkw_mcf->calculate_server_flow(candidates_cp);
    sort(candidates.begin(), candidates.end(), candidate_server_out_flow_cmp_less());
    /*for(int i = 0; i < node_num; i++)
    {
        cout<<candidates[i]->node_id<<" "<<candidates[i]->server_out_flow<<endl;
    }*/

}

bool OptSearch::calculate_candidates_pass_cost()
{
    run_simplex_mcf(ONLY_RUN);
    mcf_simplex->calculate_node_pass_flow();
    for(int i = 0; i < node_num; i++)
    {
        candidates_cp[i]->pass_cost = simplex_cost_array[i];
        //cout<<i<<" "<<simplex_cost_array[i]<<endl;
    }
    //cout<<zkw_mcf->max_flow<<" "<<zkw_mcf->sum_demand<<endl;
    return true;
}

bool OptSearch::calculate_candidates_pass_flow()
{
    run_simplex_mcf(ONLY_RUN);
    mcf_simplex->calculate_node_pass_flow();
    for(int i = 0; i < node_num; i++)
    {
        candidates_cp[i]->pass_flow = simplex_flow_array[i];
        //cout<<i<<" "<<simplex_cost_array[i]<<endl;
    }
    //cout<<zkw_mcf->max_flow<<" "<<zkw_mcf->sum_demand<<endl;
    return true;
}

bool OptSearch::calculate_server_cost(vector<ServerCost>& server_costs)
{
    mcf_simplex->init_simplex_cost_matrix();
    run_simplex_mcf(ONLY_RUN);
    mcf_simplex->calculate_server_supply();

    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        for(int j = 0; j < node_num; j++)
        {
            if(simplex_flow_matrix[i][j])
            {
                server_costs.push_back(ServerCost(i, j, simplex_cost_matrix[i][j]));
                //cout<<i<<" "<<j<<" "<<simplex_cost_matrix[i][j]<<endl;
            }
        }
    }
    sort(server_costs.begin(), server_costs.end(), server_cost_cmp_greater());
    cout<<"server cost: "<<server_costs.size()<<endl;
    /*if(tag == 0)
    {
        for(int i = 0; i < server_costs.size(); i++)
        {
            cout<<server_costs[i].server_id<<" "<<server_costs[i].customer_id<<" "<<server_costs[i].cost<<endl;
        }
    }*/
}

void OptSearch::calculate_candidates_overflow_cost()
{
    run_simplex_mcf(ONLY_RUN);
    int server_id, flow, level, overflow, over_cost;
    for(int i = 0; i < mcf_simplex->server_flow.size(); i++)
    {
        server_id = mcf_simplex->server_flow[i].first;
        flow = mcf_simplex->server_flow[i].second;
        level = g_server_level_index[flow];
        //cout<<"server: "<<flow<<" "<<server_id<<" "<<level<<endl;
        overflow = flow - g_server_level[level - 1].power;
        over_cost = g_server_level[level].cost - g_server_level[level - 1].cost;
        candidates_cp[server_id]->overflow_cost = over_cost / (overflow * 1.0);
    }
}

void OptSearch::count_used_edges()
{
    memset(edge_used, 0, sizeof(bool) * graph->edge_num);

    memset(servers, -1, sizeof(int) * node_num);
    for(int i = 0; i < node_num; i++)
    {
        servers[i] = level_num - 1;
        run_simplex_mcf(ONLY_RUN);
        //if(mcf_simplex->max_flow >= 150)
        //cout<<i<<" "<<mcf_simplex->max_flow<<endl;
        mcf_simplex->calculate_edge_used();
        servers[i] = -1;
    }

    int cnt = 0;
    for(int i = 0; i < graph->edge_num; i++)
    {
        cnt += edge_used[i];
    }
    cout<<"used edge cnt: "<<cnt<<endl;
}

int OptSearch::choose_server_by_node_demand(double percent)
{
    sort(candidates.begin(), candidates.end(), candidate_demand_cmp_greater());

    int cus_num = int(customer_num * percent);
    int cus_cnt = 0, node_id;
    for(int i = 0; i < cus_num; i++)
    {
        node_id = candidates[i]->node_id;
        servers[node_id] = level_num - 1;
    }

    //sort(increase_candidates.begin(), increase_candidates.end(), candidate_demand_cmp_greater());
    //for(int i = 0; i < node_num; i++)
    //cout<<increase_candidates[i]->node_id<<endl;
    return cus_num;
}

int OptSearch::choose_server_by_node_max_flow(bool re_cal, double percent)
{
    if(re_cal)  calculate_candidates_neighbor();
    sort(candidates.begin(), candidates.end(), candidate_flow_cmp_greater());

    int middle_num = int(node_num * percent);
    int node_id;
    for(int i = 0; i < middle_num; i++)
    {
        node_id = candidates[i]->node_id;
        servers[node_id] = level_num - 1;
    }
    return middle_num;
}

void OptSearch::choose_server_by_pass_flow(bool re_cal, double percent)
{
    if(re_cal) calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());
    int num = node_num * percent, node_id = 0;
    for(int i = 0; i < num; i++)
    {
        node_id = candidates[i]->node_id;
        servers[node_id] = level_num - 1;
    }
}

void OptSearch::construct_first_solution_combine_demand_and_flow()
{
    double p0 = 0.1, p1 = 0.1;
    choose_server_by_node_demand(p0);
    choose_server_by_node_max_flow(true, p1);
    bool is_feasible = run_simplex_mcf(MAX_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
    while(!is_feasible)
    {
        p0 += 0.1;
        p1 += 0.1;
        cout<<p0<<" "<<p1<<endl;
        choose_server_by_node_demand(p0);
        choose_server_by_node_max_flow(false, p1);
        memcpy(pre_servers, servers, sizeof(int) * node_num);
        is_feasible = run_simplex_mcf(MAX_LEVEL_FLOW_COST);
        cout<<is_feasible<<" "<<global_min_cost<<endl;
    }
}

void OptSearch::construct_first_solution_by_min_server_cost()
{
    int max_lv_cost = g_server_level[level_num - 1].cost;

    memset(servers, -1, sizeof(int) * node_num);
    for(int i = 0; i < node_num; i++)
    {
        servers[i] = level_num - 1;
        run_simplex_mcf(ONLY_RUN);
        //if(mcf_simplex->max_flow >= 150)
        //cout<<i<<" "<<mcf_simplex->max_flow<<endl;
        mcf_simplex->calculate_server_supply();
        servers[i] = -1;
    }

    for(int i = 0; i < node_num; i++)
    {
        memcpy(opt_cost_matrix[i], simplex_cost_matrix[i], sizeof(int)* node_num);
        memcpy(opt_flow_matrix[i], simplex_flow_matrix[i], sizeof(int)* node_num);
    }

    for(int i = 0; i < node_num; i++)
    {
        int max_flow = 0;
        for(int j = 0; j < node_num; j++)
        {
            if(simplex_flow_matrix[i][j] != 0 && simplex_flow_matrix[i][j] == g_customer_need[g_node_customer[j]])
            {
                simplex_cost_matrix_cp[i][j] = simplex_cost_matrix[i][j];
            }
            else
            {
                simplex_cost_matrix_cp[i][j] = INF;
            }

            if(simplex_flow_matrix[i][j] != 0 && simplex_flow_matrix[i][j] == g_customer_need[g_node_customer[j]]
                && simplex_cost_matrix[i][j] < max_lv_cost)
            {
                max_flow += simplex_flow_matrix[i][j];
            }
            else
            {
                simplex_cost_matrix[i][j] = INF;
                simplex_flow_matrix[i][j] = 0;
            }
        }
        if(max_flow == 0) continue;
        for(int j = 0; j < node_num; j++)
        {
            if(simplex_flow_matrix[i][j] == 0) continue;
            simplex_cost_matrix[i][j] += 1.0 * max_lv_cost / max_flow * simplex_flow_matrix[i][j];
            //simplex_cost_matrix[i][j] += 1.0 * g_server_level_index[max_flow] / max_flow * simplex_flow_matrix[i][j];
            //cout<<i<<" "<<" "<<j<<" "<<mcf_cost_matrix[i][j]<<" "<<mcf_flow_matrix[i][j]<<" "<<max_flow<<endl;
        }
    }

    int server_id;
    for(int j = 0; j < node_num; j++)
    {
        if(g_node_customer[j] == -1) continue;

        vector<pair<int, int>> myarray;
        for(int i = 0; i < node_num; i++)
        {
            if(simplex_cost_matrix[i][j] != INF)
                myarray.push_back(make_pair(simplex_cost_matrix[i][j], i));
        }
        sort(myarray.begin(), myarray.end());
        for(int i = 0; i < 2 && i < myarray.size(); i++)
        {
            server_id = myarray[i].second;
            servers[server_id] = level_num - 1;
            //cout<<j<<" "<<myarray[i].first<<" "<<myarray[i].second<<" "<<endl;
        }
        if(myarray.size() == 0)
        {
            servers[j] = level_num - 1;
        }
    }

    /*int cnt = 0;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] != -1)
        {
            //cnt++;
            cout<<i<<" ";
        }
    }
    cout<<cnt<<endl;*/

    bool is_feasible = run_simplex_mcf(NO_UPDATE_LEVEL);
    if(is_feasible)
    {
        cout<<"construct succeed!!!"<<" "<<global_min_cost<<endl;
    }
    else
    {
        global_min_cost = INF;
    }

    init_supply_costs();
}
//update_type 0 最大等级跑，按流量更新服务器
//run_type  1    服务器等级跑，按更新更新服务器
//run type  2   服务器等级跑，不做更新

bool OptSearch::run_simplex_mcf(int run_type)
{
    end_time = clock();
    if((end_time - start_time) * 1.0 / CLOCKS_PER_SEC > 88)
        time_out = true;
    mcf_cnt++;
    bool is_feasible;
    if(run_type == MAX_LEVEL_FLOW_COST)
    {
        mcf_simplex->init(this->servers, 1);
        is_feasible = mcf_simplex->solve();

        if(is_feasible)
        {
            memset(servers, -1, sizeof(int) * node_num);
            global_min_cost = mcf_simplex->calculate_total_cost(servers, 0);
        }
    }
    else if(run_type == SERVER_LEVEL_FLOW_COST)
    {
        mcf_simplex->init(this->servers, 0);
        is_feasible = mcf_simplex->solve();
        if(is_feasible)
        {
            memset(servers, -1, sizeof(int) * node_num);
            global_min_cost = mcf_simplex->calculate_total_cost(servers, 0);
        }
    }
    else if(run_type == ONLY_RUN)
    {
        mcf_simplex->init(this->servers, 0);
        is_feasible = mcf_simplex->solve();
    }
    else if(run_type == NO_UPDATE_LEVEL)
    {
        mcf_simplex->init(this->servers, 0);
        is_feasible = mcf_simplex->solve();
        if(is_feasible)
        {
            global_min_cost = mcf_simplex->calculate_total_cost(servers, 1);
        }
    }

    return is_feasible;
}

void OptSearch::topo_strategy()
{
    //count_used_edges();
    if(topo_lv == 0)
    {
        deal_with_topo_lv0_one();
        //zdd_search();
        //deal_with_topo_lv0_two();
        //zdd_search();
        //deal_with_topo_lv0_two();
    }
    else if(topo_lv == 1)
    {
        deal_with_topo_lv1_one();
        //construct_simplex_graph_one();
        //deal_with_topo_lv1_two();
        //deal_with_topo_lv1_two();
        //deal_with_topo_lv0_one();
        //deal_with_topo_lv0_one();
    }
    cout<<"mcf cnt: "<<mcf_cnt<<endl;
    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;

}

void OptSearch::deal_with_topo_lv0_zero()
{
    construct_first_solution_combine_demand_and_flow();
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] != -1) servers[i] = level_num - 1;
    }
    run_simplex_mcf(NO_UPDATE_LEVEL);
    decrease_server_by_server_out_flow_no_adjust_level();
    //decrease_server_by_server_outflow_max_level();
    //increase_server_by_node_pass_flow_max_level();
    //show_detail_info();
    //memcpy(servers, pre_servers, sizeof(int) * node_num);
    bool is_feasible = run_simplex_mcf(1);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
    //show_detail_info();
    cout<<"mcf_cnt: "<<mcf_cnt<<endl;
}

void OptSearch::deal_with_topo_lv0_one()
{
    bool succeed;
    construct_first_solution_by_min_server_cost();
    decrease_server_by_server_out_flow_no_adjust_level();

    show_supply_detail();
    int cnt = 0;
    while(cnt++ < 3)
    {
        decrease_server_by_server_outflow_max_level();
        increase_server_by_node_pass_cost_max_level();
        exchange_server_with_neighbor_max_level();
        exchange_server_by_node_pass_cost_max_level();
    }

    show_supply_detail();

    cnt = 0;
    while(cnt++ < 3)
    {
        exchange_server_by_server_cost_no_adjust_level();
        exchange_server_with_neighbor_no_adjust_level();
        exchange_server_by_node_pass_cost_no_adjust_level();
        increase_server_by_node_pass_cost_no_adjust_level();
        decrease_server_by_server_out_flow_no_adjust_level();
    }

    show_supply_detail();
    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

    //level_num = level_num_cp;
    //graph->level_num = level_num_cp;

    cnt = 0;
    while(cnt++ < 2)
    {
        decrease_server_by_server_outflow();
        increase_server_by_node_pass_cost();
    }
/**********************/
    /*succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }
    zdd_up_server_level();

    cnt = 0;
    while(cnt++ < 3)
    {

        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        exchange_server_with_neighbor();
        zdd_up_server_level();
        //exchange_server_by_node_pass_cost();

        //decrease_server_by_server_outflow();
        //increase_server_by_node_pass_cost();
    }*/
/**********************/

    /*cnt = 0;
    while(cnt++ < 2)
    {

        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        exchange_server_with_neighbor();

        succeed = true;
        while(succeed)
        {
            succeed = zdd_down_server_level();
        }

        zdd_up_server_level();
        //exchange_server_by_node_pass_cost();

        //decrease_server_by_server_outflow();
        //increase_server_by_node_pass_cost();
    }*/
/**********************/

    level_num = level_num_cp;
    graph->level_num = level_num_cp;

    cnt = 0;
    while(cnt++ < 2)
    {

        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        exchange_server_with_neighbor();

        succeed = true;
        while(succeed)
        {
            succeed = zdd_down_server_level();
        }

        zdd_up_server_level();
        //exchange_server_by_node_pass_cost();

        //decrease_server_by_server_outflow();
        //increase_server_by_node_pass_cost();
    }

    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
   // show_supply_detail();

}

void OptSearch::deal_with_topo_lv0_two()
{
    //int* best_servers = n
    bool succeed;
    //construct_first_solution_by_simplex_graph();
    construct_first_solution_by_min_server_cost();
    decrease_server_by_server_outflow_max_level();
    increase_server_by_node_pass_cost_max_level();
    //show_supply_detail();

    int cnt = 0;
    while(cnt++ < 2)
    {
        decrease_server_by_server_outflow_max_level();
        increase_server_by_node_pass_cost_max_level();
        //exchange_server_with_neighbor_max_level();
        //exchange_server_by_node_pass_cost_max_level();
    }

    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

    //global_min_cost = INF;
        //decrease_server_by_server_outflow_max_level();
    //increase_server_by_node_pass_cost_max_level();
    //decrease_server_by_server_outflow_max_level();

    show_supply_detail();

    succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }
    zdd_up_server_level();
    //show_supply_detail();
    cnt = 0;
    while(cnt++ < 4)
    {

        exchange_server_by_node_pass_cost();
        exchange_server_with_neighbor();
        zdd_up_server_level();
    }

    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
    show_supply_detail();
    //show_supply_detail();*/
    //is_feasible = run_simplex_mcf(NO_UPDATE_LEVEL);
    //cout<<is_feasible<<" "<<global_min_cost<<endl;
}

void OptSearch::punish_strategy()
{
    int* best_servers = new int[node_num];
    memcpy(best_servers, servers, sizeof(int)*node_num);

    vector<int> punish_array(node_num, 0);
    int punish_num = 200;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] != -1)
        {
            g_node_cost[i] += punish_num;
            punish_array[i] = punish_num;
            global_min_cost += punish_num;
        }
    }

    int cnt = 0;
    bool succeed;
    while(cnt++ < 2)
    {
        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        exchange_server_with_neighbor();
        //exchange_server_by_node_pass_cost();

        decrease_server_by_server_outflow();
        increase_server_by_node_pass_cost();

        succeed = true;
        while(succeed)
        {
            succeed = zdd_down_server_level();
        }

        zdd_up_server_level();
    }

    for(int i = 0; i < node_num; i++)
    {
        g_node_cost[i] -= punish_array[i];
    }

    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<"after punish: "<<global_min_cost<<endl;
    show_supply_detail();

    for(int i = 0; i < node_num; i++)
    {
        if(best_servers[i] != -1)
        {
            servers[i] = best_servers[i];
        }
    }

    run_simplex_mcf(MAX_LEVEL_FLOW_COST);

    cnt = 0;
    while(cnt++ < 2)
    {
        decrease_server_by_server_outflow();
        increase_server_by_node_pass_cost();
    }

    cnt = 0;
    while(cnt++ < 2)
    {

        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        exchange_server_with_neighbor();
        //exchange_server_by_node_pass_cost();

        decrease_server_by_server_outflow();
        increase_server_by_node_pass_cost();
    }

    succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }

    zdd_up_server_level();
    is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
}

void OptSearch::zdd_search()
{
    construct_first_solution_by_min_server_cost();
    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

    int cnt = 0;
    while(cnt++ < 10)
    {
        decrease_server_by_server_outflow_max_level();
        increase_server_by_node_pass_cost_max_level();
    }

    bool succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }

    zdd_up_server_level();
    cnt = 0;
    while(cnt++ < 2)
    {
        //
        //
        exchange_server_with_neighbor();
        exchange_server_by_node_pass_cost();
        exchange_server_by_server_cost();
        //decrease_server_by_server_outflow();
        //increase_server_by_node_pass_cost();
    }

    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;
    show_supply_detail();
}

void OptSearch::show_supply_detail()
{
    int cnt = 0;
    vector<int> level_cnts(10, 0);
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] != -1)
        {
            cnt++;
            cout<<i<<" ";
            level_cnts[servers[i]]++;
        }
    }
    cout<<"server cnt: "<<cnt<<endl;

    for(int i = 0; i < 10; i++)
    {
        cout<<i<<" "<<level_cnts[i]<<endl;
    }
    /*calculate_candidates_pass_cost();
    for(int i = 0; i < node_num; i++)
    {
        cout<<candidates_cp[i]->node_id<<" "<<candidates_cp[i]->pass_cost<<endl;
    }*/

}

void OptSearch::deal_with_topo_lv1_zero()
{
    bool succeed;
    construct_first_solution_by_min_server_cost();
    decrease_server_by_server_out_flow_no_adjust_level();

    int cnt = 0;
    while(cnt++ < 1)
    {
        //exchange_server_by_server_cost_no_adjust_level();
        exchange_server_with_neighbor_no_adjust_level();
        exchange_server_by_node_pass_cost_no_adjust_level();
        increase_server_by_node_pass_cost_no_adjust_level();
        decrease_server_by_server_out_flow_no_adjust_level();
    }

    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    //decrease_server_by_server_outflow();
    increase_server_by_node_pass_cost();

    succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }

    cnt = 0;
    while(cnt++ < 1)
    {
        exchange_server_by_node_pass_cost();
        //exchange_server_by_server_cost();
        exchange_server_with_neighbor();
        //decrease_server_by_server_outflow();
        //increase_server_by_node_pass_cost();
    }
}

void OptSearch::deal_with_topo_lv1_one()
{
    bool succeed;
    construct_first_solution_by_min_server_cost();
    decrease_server_by_server_out_flow_no_adjust_level();
    //show_supply_detail();

    //run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    int cnt = 0;
    while(cnt++ < 1)
    {
        exchange_server_with_neighbor_no_adjust_level();
        decrease_server_by_server_out_flow_no_adjust_level();
        //increase_server_by_node_pass_cost_no_adjust_level();
    }
    //show_supply_detail();
    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    //decrease_server_by_server_outflow();
    increase_server_by_node_pass_cost();
    //show_supply_detail();
    //chj_down_server_level();
    succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();
    }
    //show_supply_detail();

    graph->level_num = level_num_cp;
    level_num = level_num_cp;

    cnt = 0;
    greedy_level = 1;
    while(cnt++ < 10)
    {
        exchange_server_with_neighbor();
        //exchange_server_by_node_pass_cost_lv0();
        if(time_out) break;

    }
    greedy_level = 0;


    exchange_server_with_neighbor();

    //graph->level_num = level_num_cp;
    //level_num = level_num_cp;

    chj_up_server_level();
    show_supply_detail();
}

void OptSearch::deal_with_topo_lv1_two()
{
    int cnt;
    bool succeed;
    //construct_first_solution_by_simplex_graph();
    construct_first_solution_by_min_server_cost();
    decrease_server_by_server_outflow_max_level();
    increase_server_by_node_pass_cost_max_level();

    //decrease_server_by_server_outflow_max_level();
    //increase_server_by_node_pass_cost_max_level();
    show_supply_detail();

    cnt = 0;
    greedy_level = 1;
    while(cnt++ < 1)
    {
        exchange_server_with_neighbor_max_level();
        //increase_server_by_node_pass_cost();
        decrease_server_by_server_outflow_max_level();
        increase_server_by_node_pass_cost_max_level();
    }
    show_supply_detail();



    //increase_server_by_node_pass_cost();
    show_supply_detail();
    succeed = true;
    while(succeed)
    {
        succeed = zdd_down_server_level();

    }

    chj_up_server_level();

    show_supply_detail();
    cnt = 0;
    greedy_level = 1;
    while(cnt++ < 4)
    {

        exchange_server_with_neighbor();
        if(time_out) break;

    }
    greedy_level = 0;
    exchange_server_with_neighbor();
    //zdd_up_server_level();
    chj_up_server_level();

    show_supply_detail();

}

void OptSearch::lv0_add_server(int type)
{
    if(type == 0)
    {
        choose_server_by_pass_flow(true, 0.1);
    }
    global_min_cost = INF;
    run_simplex_mcf(MAX_LEVEL_FLOW_COST);
}

bool OptSearch::show_detail_info()
{
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] != -1)
        {
            cout<<i<<" "<<servers[i]<<endl;
        }
    }
    cout<<endl<<"min cost: "<<global_min_cost<<endl;

}

bool OptSearch::decrease_server_by_server_outflow_max_level()
{
    calculate_candidate_out_flow();

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id, tmp_lv;
    bool is_feasible;
    for(int i = 0; i < node_num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        servers[node_id] = -1;
        is_feasible = run_simplex_mcf(0);
        //cout<<node_id<<" "<<is_feasible<<" "<<zkw_mcf->min_cost<<" "<<zkw_mcf->max_flow<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
        //break;
    }

    cout<<"min cost after decrease_server_by_server_outflow_max_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::decrease_server_by_server_out_flow_no_adjust_level()
{
    calculate_candidate_out_flow();

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id, tmp_lv;
    bool is_feasible;
    for(int i = 0; i < node_num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] == -1) continue;

        tmp_lv = servers[node_id];
        servers[node_id] = -1;
        is_feasible = run_simplex_mcf(NO_UPDATE_LEVEL);
        //cout<<node_id<<" "<<is_feasible<<" "<<mcf_simplex->min_cost<<" "<<mcf_simplex->max_flow<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            servers[node_id] = tmp_lv;
            global_min_cost = best_cost;
        }
        //break;
    }

    cout<<"min cost after decrease_server_by_server_out_flow_no_adjust_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::decrease_server_by_server_outflow()
{
    calculate_candidate_out_flow();

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id, tmp_lv;
    bool is_feasible;
    for(int i = 0; i < node_num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        servers[node_id] = -1;
        is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
        //break;
    }

    cout<<"min cost after decrease_server_by_server_out_flow: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::increase_server_by_node_pass_cost_max_level()
{
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id;
    bool is_feasible;
    for(int i = 0; i < node_num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] != -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        servers[node_id] = level_num - 1;
        is_feasible = run_simplex_mcf(0);
        //cout<<node_id<<" "<<is_feasible<<" "<<zkw_mcf->min_cost<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
    }
    cout<<"min cost after increase_server_by_node_pass_flow_max_level: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::increase_server_by_node_pass_cost_no_adjust_level()
{
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id;
    bool is_feasible;
    int num = node_num * 1;
    for(int i = 0; i < num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] != -1) continue;
        //if(candidates[i]->pass_flow == 0) break;

        servers[node_id] = level_num - 1;
        is_feasible = run_simplex_mcf(NO_UPDATE_LEVEL);
        //cout<<node_id<<" "<<is_feasible<<" "<<zkw_mcf->min_cost<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            servers[node_id] = -1;
            global_min_cost = best_cost;
        }
    }
    cout<<"min cost after increase_server_by_node_pass_flow: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::increase_server_by_node_pass_cost()
{
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id;
    bool is_feasible;

    int num = node_num * 1;
    for(int i = 0; i < num; i++)
    {
        node_id = candidates[i]->node_id;
        if(servers[node_id] != -1) continue;
        //if(candidates[i]->pass_flow == 0) break;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        servers[node_id] = level_num - 1;
        is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
        //cout<<node_id<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
    }
    cout<<"min cost after increase_server_by_node_pass_flow: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

int OptSearch::exchange_node_no_adjust_level(int in_id, int in_lv, int out_id)
{
    if(servers[in_id] != -1 || servers[out_id] == -1)
    {
        cout<<"exchange_node error"<<endl;
        return 2;
    }

    int best_cost = global_min_cost;
    int tmp_lv = servers[out_id];

    servers[in_id] = in_lv;
    servers[out_id] = -1;

    bool is_feasible = run_simplex_mcf(NO_UPDATE_LEVEL);

    if(is_feasible)
    {
        if(global_min_cost < best_cost)
        {
            return 0;
        }
        else
        {

            servers[out_id] = tmp_lv;
            servers[in_id] = -1;
            global_min_cost = best_cost;
            return 1;
        }
    }
    else
    {
        servers[out_id] = tmp_lv;
        servers[in_id] = -1;
        global_min_cost = best_cost;
        return 2;
    }

}

bool OptSearch::exchange_one_server_with_other_candidates_no_adjust_level(int out_id)
{
    int up;
    if(topo_lv == 0) up = 20;
    else if(topo_lv == 1) up = 20;
    else up = 20;

    int in_lv, ret, real_lv, in_id, cnt = 0;
    bool succeed = false;
    for(int i = 0; i < node_num; i++)
    {
        //if(increase_candidates_cp[i] == empty_candidate) continue;
        in_id = candidates[i]->node_id;
        if(servers[in_id] != -1) continue;
        if(topo_lv <= 2)
        {
            cnt++;
            if(cnt >= up) break;
        }

        ret = exchange_node_no_adjust_level(in_id, servers[out_id], out_id);
        if(ret == 2 || ret == 1) continue;
        if(ret == 0)
        {
            succeed = true;
            break;
        }
        //ret = exchange_node(in_id, real_lv - 1, out_id, real_lv);
        //if(ret == 0) succeed = true;
        if(succeed) break;
    }

    return succeed;
}

bool OptSearch::exchange_server_by_server_cost_no_adjust_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;

    vector<ServerCost> server_costs;
    calculate_server_cost(server_costs);

    int server_id, customer_id, in_id, ret, min_in_id, tmp_cost, tmp_lv;
    bool exchange;
    for(int i = 0; i < server_costs.size(); i++)
    {
        server_id = server_costs[i].server_id;
        customer_id = server_costs[i].customer_id;
        if(servers[server_id] == -1) continue;

        min_in_id = -1;
        tmp_cost = global_min_cost;
        tmp_lv = servers[server_id];
        exchange = false;

        for(int j = 0; j < supply_costs[customer_id].size(); j++)
        {
            in_id = supply_costs[customer_id][j].server_id;
            if(servers[in_id] != -1) continue;

            ret = exchange_node_no_adjust_level(in_id, servers[server_id], server_id);

            if(ret == 0 && global_min_cost < best_cost)
            {
                best_cost = global_min_cost;
                min_in_id = in_id;
                exchange = true;
            }

            servers[server_id] = tmp_lv;
            servers[in_id] = -1;
            global_min_cost = tmp_cost;
        }

        if(min_in_id != -1)
        {
            //cout<<min_in_id<<" "<<server_id<<endl;
            servers[min_in_id] = tmp_lv;
            servers[server_id] = -1;
            global_min_cost = best_cost;
        }
        if(exchange) break;
    }

    cout<<"min cost after exchange_server_by_server_cost_no_adjust_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_with_neighbor_no_adjust_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int in_id, out_id, ret, lv;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        out_id = i;
        for(int j = g_node_head[i]; j != -1; j = g_edge_next[j])
        {
            in_id = g_edge_des[j];
            if(servers[in_id] != -1) continue;
            ret = exchange_node_no_adjust_level(in_id, servers[out_id], out_id);
            if(ret == 0) break;
        }
    }

    cout<<"min cost after exchange_server_with_neighbor_no_adjust_level: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_with_neighbor_max_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int in_id, out_id, ret, lv;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        out_id = i;
        for(int j = g_node_head[i]; j != -1; j = g_edge_next[j])
        {
            in_id = g_edge_des[j];
            if(servers[in_id] != -1) continue;
            ret = exchange_node_max_level(in_id, servers[out_id], out_id);
            if(ret == 0) break;
        }
    }

    cout<<"min cost after exchange_server_with_neighbor_no_adjust_level: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_by_node_pass_cost_no_adjust_level()
{
    //cout<<111<<endl;
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int begin_cost = global_min_cost;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        exchange_one_server_with_other_candidates_no_adjust_level(i);
    }

    cout<<"min cost after exchange_server_by_node_pass_cost_no_adjust_level: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_by_node_pass_cost_max_level()
{
    //cout<<111<<endl;
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int begin_cost = global_min_cost;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        exchange_one_server_with_other_candidates_max_level(i);
    }

    cout<<"min cost after exchange_server_by_node_pass_cost_no_adjust_level: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_by_node_pass_cost_lv0()
{
    //cout<<111<<endl;
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int begin_cost = global_min_cost;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        exchange_one_server_with_other_candidates(i);
    }

    cout<<"min cost after exchange_server_by_node_pass_cost lv0: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_by_node_pass_cost()
{
    calculate_candidates_pass_cost();
    sort(candidates.begin(), candidates.end(), candidate_pass_cost_cmp_greater());

    int begin_cost = global_min_cost;
    int in_id;
    for(int i = 0; i < 100 && i < node_num; i++)
    {
        if(time_out) break;
        in_id = candidates[i]->node_id;
        //if(increase_candidates_cp[in_id] == empty_candidate) continue;
        if(servers[in_id] != -1) continue;
        exchange_one_node_with_all_server(in_id);
    }

    cout<<"min cost after exchange_server_by_node_pass_cost: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_one_node_with_all_server(int in_id)
{
    int in_lv, ret, real_lv, out_id;
    bool succeed = false;
    for(int i = 0; i < node_num; i++)
    {
        if(time_out) break;
        if(servers[i] == -1) continue;
        out_id = i;
        ret = exchange_node(in_id, servers[out_id], out_id, real_lv);
        if(ret == 2 || ret == 1) continue;
        if(ret == 0)
        {
            succeed = true;
            break;
        }
        //ret = exchange_node(in_id, real_lv - 1, out_id, real_lv);
       // if(ret == 0) succeed = true;
        if(succeed) break;
    }

    return succeed;
}


int OptSearch::exchange_node(int in_id, int in_lv, int out_id, int& real_lv)
{
    if(servers[in_id] != -1 || servers[out_id] == -1)
    {
        cout<<"exchange_node error"<<endl;
        return 2;
    }

    int best_cost = global_min_cost;
    int tmp_lv = servers[out_id];

    memcpy(pre_servers, servers, sizeof(int) * node_num);


    servers[in_id] = in_lv;
    servers[out_id] = -1;
    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

    if(is_feasible)
    {
        real_lv = servers[in_id];
        if(global_min_cost < best_cost)
        {
            return 0;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
            return 1;
        }
    }
    else
    {
        servers[out_id] = tmp_lv;
        servers[in_id] = -1;
        global_min_cost = best_cost;
        return 2;
    }

}

int OptSearch::exchange_node_max_level(int in_id, int in_lv, int out_id)
{
    if(servers[in_id] != -1 || servers[out_id] == -1)
    {
        cout<<"exchange_node error"<<endl;
        return 2;
    }

    int best_cost = global_min_cost;
    int tmp_lv = servers[out_id];

    memcpy(pre_servers, servers, sizeof(int) * node_num);


    servers[in_id] = in_lv;
    servers[out_id] = -1;
    bool is_feasible = run_simplex_mcf(MAX_LEVEL_FLOW_COST);

    if(is_feasible)
    {
        if(global_min_cost < best_cost)
        {
            return 0;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
            return 1;
        }
    }
    else
    {
        servers[out_id] = tmp_lv;
        servers[in_id] = -1;
        global_min_cost = best_cost;
        return 2;
    }

}

bool OptSearch::exchange_server_by_server_cost()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;

    int* best_servers = new int[node_num];

    vector<ServerCost> server_costs;
    calculate_server_cost(server_costs);

    int server_id, customer_id, in_id, ret, min_in_id, tmp_cost, tmp_lv;
    bool exchange;
    for(int i = 0; i < server_costs.size(); i++)
    {
        server_id = server_costs[i].server_id;
        customer_id = server_costs[i].customer_id;
        if(servers[server_id] == -1) continue;

        min_in_id = -1;
        tmp_cost = global_min_cost;
        memcpy(pre_servers, servers, sizeof(int) * node_num);
        exchange = false;

        for(int j = 0; j < supply_costs[customer_id].size(); j++)
        {
            in_id = supply_costs[customer_id][j].server_id;
            if(servers[in_id] != -1) continue;
            //cout<<in_id<<" "<<server_id<<endl;

            ret = exchange_node(in_id, servers[server_id], server_id, tmp_lv);

            if(ret == 0 && global_min_cost < best_cost)
            {
                best_cost = global_min_cost;
                memcpy(best_servers, servers, sizeof(int) * node_num);
                exchange = true;
            }

            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = tmp_cost;
        }

        if(min_in_id != -1)
        {
            //cout<<min_in_id<<" "<<server_id<<endl;
            memcpy(servers, best_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
        if(exchange) break;
    }

    delete best_servers;
    cout<<"min cost after exchange_server_by_server_cost: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_server_with_neighbor()
{
    int neighbor_cnt = 0;
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int in_id, out_id, ret, lv;
    for(int i = 0; i < node_num; i++)
    {
        if(servers[i] == -1) continue;
        out_id = i;
        for(int j = g_node_head[i]; j != -1; j = g_edge_next[j])
        {
            if(time_out) break;
            in_id = g_edge_des[j];
            if(servers[in_id] != -1) continue;
            if(greedy_level == 0)
            {
                neighbor_cnt++;
                ret = exchange_node(in_id, servers[out_id], out_id, lv);
                if(ret == 0) break;
            }
            else if(greedy_level == 1)
            {
                if(g_neighbor_succeed[out_id][in_id] == 0) continue;
                neighbor_cnt++;
                ret = exchange_node(in_id, servers[out_id], out_id, lv);
                if(ret == 0)
                {
                    g_neighbor_succeed[out_id][in_id] = 1;
                    break;
                }
                else
                {
                    g_neighbor_succeed[out_id][in_id] = 0;
                }

            }
        }
        if(time_out) break;
    }
    cout<<"neighbor_cnt: "<<neighbor_cnt<<endl;
    cout<<"min cost after exchange_server_with_neighbor: "<<global_min_cost<<endl;

    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::exchange_one_server_with_other_candidates(int out_id)
{
    int up;
    if(topo_lv == 0) up = 20;
    else if(topo_lv == 1) up = 20;
    else up = 20;

    int in_lv, ret, real_lv, in_id, cnt = 0;
    bool succeed = false;
    for(int i = 0; i < node_num; i++)
    {
        //if(increase_candidates_cp[i] == empty_candidate) continue;
        in_id = candidates[i]->node_id;
        if(servers[in_id] != -1) continue;
        if(topo_lv <= 2)
        {
            cnt++;
            if(cnt >= up) break;
        }

        ret = exchange_node(in_id, servers[out_id], out_id, real_lv);
        if(ret == 2 || ret == 1) continue;
        if(ret == 0)
        {
            succeed = true;
            break;
        }
        //ret = exchange_node(in_id, real_lv - 1, out_id, real_lv);
        //if(ret == 0) succeed = true;
        if(succeed) break;
    }

    return succeed;
}

bool OptSearch::exchange_one_server_with_other_candidates_max_level(int out_id)
{
    int up;
    if(topo_lv == 0) up = 20;
    else if(topo_lv == 1) up = 20;
    else up = 20;

    int in_lv, ret, real_lv, in_id, cnt = 0;
    bool succeed = false;
    for(int i = 0; i < node_num; i++)
    {
        //if(increase_candidates_cp[i] == empty_candidate) continue;
        in_id = candidates[i]->node_id;
        if(servers[in_id] != -1) continue;
        if(topo_lv <= 2)
        {
            cnt++;
            if(cnt >= up) break;
        }

        ret = exchange_node_max_level(in_id, servers[out_id], out_id);
        if(ret == 2 || ret == 1) continue;
        if(ret == 0)
        {
            succeed = true;
            break;
        }
        //ret = exchange_node(in_id, real_lv - 1, out_id, real_lv);
        //if(ret == 0) succeed = true;
        if(succeed) break;
    }

    return succeed;
}

bool OptSearch::zdd_down_server_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id, tmp_lv, min_down_id = -1;
    bool is_feasible;

    for(int i = 0; i < node_num; i++)
    {
        if(time_out) break;
        node_id = i;
        if(servers[node_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);
        servers[node_id]--;
        is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
        //cout<<node_id<<" "<<is_feasible<<" "<<zkw_mcf->min_cost<<" "<<zkw_mcf->max_flow<<" "<<global_min_cost<<endl;

        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
            min_down_id = node_id;
        }

        global_min_cost = begin_cost;
        memcpy(servers, pre_servers, sizeof(int) * node_num);
        //break;
    }

    if(min_down_id != -1)
    {
        servers[min_down_id]--;
        run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    }
    cout<<"min cost after zdd_down_server_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::chj_down_server_level()
{
    int node_id, best_cost = global_min_cost, begin_cost = global_min_cost;
    bool changed, is_feasible;
    while(true)
    {
        calculate_candidates_overflow_cost();
        sort(candidates.begin(), candidates.end(), candidate_overflow_cmp_greater());
        changed = false;
        for(int i = 0; i < node_num; i++)
        {
            node_id = candidates[i]->node_id;
            if(candidates[i]->overflow_cost <= 0.0 || servers[node_id] == -1) break;
            //cout<<node_id<<" "<<candidates[i]->overflow_cost<<endl;
            memcpy(pre_servers, servers, sizeof(int) * node_num);

            servers[node_id]--;
            is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

            if(is_feasible && global_min_cost < best_cost)
            {
                best_cost = global_min_cost;
                changed = true;
                break;
            }
            else
            {
                memcpy(servers, pre_servers, sizeof(int) * node_num);
                global_min_cost = best_cost;
            }
        }
        if(!changed) break;
    }

    cout<<"min cost after chj down_server_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::zdd_up_server_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int up_id, down_id, tmp_lv, min_down_id = -1;
    bool is_feasible;

    calculate_candidates_overflow_cost();
    sort(candidates.begin(), candidates.end(), candidate_overflow_cmp_greater());

    for(int i = 0; i < node_num; i++)
    {
        if(time_out) break;
        down_id = candidates[i]->node_id;
        if(servers[down_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        for(int j = 0; j < node_num; j++)
        {
            if(time_out) break;
            up_id = j;
            if(servers[up_id] == -1 || servers[up_id] == level_num - 1) continue;

            servers[up_id]++;
            servers[down_id]--;
            is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

            if(is_feasible && global_min_cost < best_cost)
            {
                best_cost = global_min_cost;
                memcpy(pre_servers, servers, sizeof(int) * node_num);
                break;
            }
            else
            {
                memcpy(servers, pre_servers, sizeof(int) * node_num);
            }
        }

    }

    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<"min cost after zdd_up_server_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::construct_first_solution_by_simplex_graph()
{
    construct_first_solution_by_min_server_cost();

    show_supply_detail();

    save_original_graph();
    construct_simplex_graph();

    decrease_server_by_server_outflow_max_level();
    increase_server_by_node_pass_cost_max_level();

    decrease_server_by_server_outflow_max_level();
    increase_server_by_node_pass_cost_max_level();

    show_supply_detail();
    bool is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<is_feasible<<" "<<global_min_cost<<endl;

    recover_original_graph();

    is_feasible = run_simplex_mcf(MAX_LEVEL_FLOW_COST);
    if(!is_feasible)
    {
        mcf_simplex->calculate_customer_in_flow();
        for(int i = 0; i < node_num; i++)
        {
            if(g_node_customer[i] != -1)
            {
                //cout<<i<<" "<<g_customer_need[g_node_customer[i]]<<" "<<simplex_flow_array[i]<<endl;
                if(g_customer_need[g_node_customer[i]] != simplex_flow_array[i])
                {
                    servers[i] = level_num - 1;
                }
            }
            else if(simplex_flow_array[i] != 0)
            {
                cout<<"error!!!: "<<i<<endl;
            }

        }
    }

    is_feasible = run_simplex_mcf(MAX_LEVEL_FLOW_COST);
    cout<<"after construct: "<<is_feasible<<" "<<global_min_cost<<endl;


}

void OptSearch::save_original_graph()
{
    edge_num_cp = graph->edge_num;
    int num =  graph->edge_num;

    memcpy(g_edge_src_cp, g_edge_src, sizeof(int) * num);
    memcpy(g_edge_des_cp, g_edge_des, sizeof(int) * num);
    memcpy(g_edge_cap_cp, g_edge_cap, sizeof(int) * num);
    memcpy(g_edge_cost_cp, g_edge_cost, sizeof(int) * num);
    memcpy(g_edge_next_cp, g_edge_next, sizeof(int) * num);
    memcpy(g_node_head_cp, g_node_head, sizeof(int) * node_num);

    memset(g_node_head, -1, sizeof(int) * node_num);
}

void OptSearch::recover_original_graph()
{
    graph->edge_num = edge_num_cp;
    int num =  graph->edge_num;

    memcpy(g_edge_src, g_edge_src_cp, sizeof(int) * num);
    memcpy(g_edge_des, g_edge_des_cp, sizeof(int) * num);
    memcpy(g_edge_cap, g_edge_cap_cp, sizeof(int) * num);
    memcpy(g_edge_cost, g_edge_cost_cp, sizeof(int) * num);
    memcpy(g_edge_next, g_edge_next_cp, sizeof(int) * num);
    memcpy(g_node_head, g_node_head_cp, sizeof(int) * node_num);

}

void OptSearch::construct_simplex_graph()
{

    int tol = 0, src, des, cap, cost;
    for(int i = 0; i < node_num; i++)
    {
        for(int j = 0; j < node_num; j++)
        {
            if(opt_flow_matrix[i][j] == 0) continue;
            src = i, des = j;
            cap = opt_flow_matrix[i][j];
            cost = double(opt_cost_matrix[i][j]) / opt_flow_matrix[i][j];

            g_edge_src[tol] = src;
            g_edge_des[tol] = des;
            g_edge_cap[tol] = cap;
            g_edge_cost[tol] = cost;
            g_edge_next[tol] = g_node_head[src];
            g_node_head[src] = tol++;
            //cout<<src<<" "<<des<<" "<<cap<<" "<<cost<<endl;
        }
    }

    graph->edge_num = tol;

}

void OptSearch::construct_simplex_graph_one()
{
    count_used_edges();
    save_original_graph();

    int tol = 0, src, des, cap, cost;
    for(int i = 0; i < graph->edge_num; i++)
    {
        if(!edge_used[i]) continue;

        if(i % 2 == 1 && edge_used[i-1]) continue;
        src = g_edge_src_cp[i];
        des = g_edge_des_cp[i];
        cost = g_edge_cost_cp[i];
        cap = g_edge_cap_cp[i];

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
    }
    graph->edge_num = tol;
    cout<<"new edge num: "<<tol<<endl;
}

bool OptSearch::chj_up_server_level()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int up_id, down_id, tmp_lv, min_down_id = -1;
    bool is_feasible;

    calculate_candidates_overflow_cost();
    sort(candidates.begin(), candidates.end(), candidate_overflow_cmp_greater());

    mcf_simplex->init_simplex_cost_matrix();
    run_simplex_mcf(ONLY_RUN);
    mcf_simplex->calculate_server_supply();

    int cnt = 0;
    for(int i = 0; i < node_num; i++)
    {
        if(time_out) break;
        down_id = candidates[i]->node_id;
        if(servers[down_id] == -1) continue;
        cnt++;
        memcpy(pre_servers, servers, sizeof(int) * node_num);

        for(int j = 0; j < node_num; j++)
        {
            if(time_out) break;
            up_id = j;
            if(servers[up_id] == -1 || servers[up_id] == level_num - 1) continue;
            if(topo_lv == 1)
            {
                bool run = false;
                for(int k = 0; k < node_num; k++)
                {
                    if(opt_flow_matrix[up_id][k] != 0 && opt_flow_matrix[down_id][k] != 0)
                    {
                        run = true;
                        break;
                    }
                }
                if(!run) continue;
            }

            servers[up_id]++;
            servers[down_id]--;
            is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);

            if(is_feasible && global_min_cost < best_cost)
            {
                best_cost = global_min_cost;
                memcpy(pre_servers, servers, sizeof(int) * node_num);
                mcf_simplex->init_simplex_cost_matrix();
                run_simplex_mcf(ONLY_RUN);
                mcf_simplex->calculate_server_supply();
                break;

            }
            else
            {
                memcpy(servers, pre_servers, sizeof(int) * node_num);
            }
        }

    }

    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<"min cost after chj_up_server_level: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

bool OptSearch::chj_down_server_level_one()
{
    int best_cost = global_min_cost, begin_cost = global_min_cost;
    int node_id, tmp_lv, min_down_id = -1;
    bool is_feasible;

    for(int i = 0; i < node_num; i++)
    {
        node_id = i;
        if(servers[node_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);
        servers[node_id]--;
        is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
        //cout<<node_id<<" "<<is_feasible<<" "<<zkw_mcf->min_cost<<" "<<zkw_mcf->max_flow<<" "<<global_min_cost<<endl;

        candidates_cp[node_id]->min_cost = global_min_cost;
        if(!is_feasible) candidates_cp[node_id]->min_cost = INF;
        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }

        global_min_cost = begin_cost;
        memcpy(servers, pre_servers, sizeof(int) * node_num);
        //break;
    }

    sort(candidates.begin(), candidates.end(), server_min_cost_cmp_less());

    for(int i = 0; i < node_num; i++)
    {
        node_id = candidates[i]->node_id;
        if(candidates[i]->min_cost == INF) continue;
        if(servers[node_id] == -1) continue;

        memcpy(pre_servers, servers, sizeof(int) * node_num);

        servers[node_id]--;
        is_feasible = run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
        if(is_feasible && global_min_cost < best_cost)
        {
            best_cost = global_min_cost;
        }
        else
        {
            memcpy(servers, pre_servers, sizeof(int) * node_num);
            global_min_cost = best_cost;
        }
    }

    run_simplex_mcf(SERVER_LEVEL_FLOW_COST);
    cout<<"min cost after chj_down_server_level_one: "<<global_min_cost<<endl;
    if(global_min_cost < begin_cost) return true;
    return false;
}

#endif // __OPT_SEARCH_H__
