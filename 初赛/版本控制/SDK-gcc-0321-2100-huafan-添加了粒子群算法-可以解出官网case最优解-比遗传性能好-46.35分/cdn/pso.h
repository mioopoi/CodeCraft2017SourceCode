#ifndef PSO_H_INCLUDED
#define PSO_H_INCLUDED

#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

#include "globals.h"
#include "min_cost_flow.h"

using namespace std;

// 离散二进制粒子群算法
// 论文: "A Novel Binary Particle Swarm Optimization", 2007
// 2017.03.21 目前的测试结果是性能比遗传好得多


struct Particle
{
    vector<bool> x;     // 位置
    vector<bool> pbest; // 个体历史最好位置
    vector<double> v0;  // 速度: 位置上的分量趋向于变为0的概率
    vector<double> v1;  // 速度: 位置上的分量趋向于变为1的概率
    vector<double> v;   // 融合速度
    int fitness;        // 当前适应度
    int pbest_fitness;
    bool feasible;
    int num;            // 1的个数（服务器的数量）
    Particle()
    {
        feasible = false;
        fitness = INF;
        pbest_fitness = INF;
        num = 0;
    }
};


class ParticleSwarmOpt
{
public:
    double w;                      // 惯性权重
    double c1, c2;                 // 学习因子
    int pop_size;                  // 群体规模
    int max_t;                     // 最大迭代次数
    Graph * graph;
    MinCostFlow *mcf;              // 最小费用流评估个体适应度
    vector<Particle*> population;
    vector<bool> gbest;            // 群体历史最好位置
    int gbest_fitness;
    ParticleSwarmOpt(Graph *graph);
    void init_population();
    void pso_search();             // 迭代主程序

private:
    int dimension;                 // 解空间维度（图中节点数量）
    int max_num_server;
    double sigmoid(double v);      // sigmoid 函数
    void init_gbest();
    void gen_particle(Particle *particle);
    void evaluate_swarm();
    void evaluate(Particle *particle);
    void update_best(Particle *particle);
    void update_swarm_velocity();
    void update_swarm_position();
    void update_velocity(Particle *particle);
    void update_position(Particle *particle);
};


ParticleSwarmOpt::ParticleSwarmOpt(Graph *graph)
{
    srand((unsigned) time(NULL));
    this->w = 0.55;
    this->c1 = 2;
    this->c2 = 2;
    this->pop_size = 50;
    this->max_t = 400;
    this->graph = graph;
    this->dimension = graph->net_node_num;
    this->max_num_server = graph->cus_node_num;
    this->mcf = new MinCostFlow(graph);
    this->init_gbest();
}

void ParticleSwarmOpt::init_gbest()
{
    // 初始化群体最优解
    // 把所有服务器放置在与消费节点直连的网络节点上
    this->gbest = vector<bool>(dimension);
    this->gbest_fitness = INF;
    for (size_t i = 0; i < graph->cus_node_num; ++i)
    {
        int cus_nodeid = graph->cus_nodes[i]->des;
        this->gbest[cus_nodeid] = true;
    }
    mcf->init(gbest);
    bool feasible = mcf->compute();
    if (feasible)
        gbest_fitness = mcf->min_cost + graph->net_fee * max_num_server;
    if (feasible)
    {
        cout << "feasible initial gbest with cost: " << gbest_fitness << endl;
    }
}

void ParticleSwarmOpt::init_population()
{
    for (int i = 0; i < pop_size; ++i)
    {
        Particle *particle = new Particle();
        gen_particle(particle);
        population.push_back(particle);
    }
}

void ParticleSwarmOpt::gen_particle(Particle *particle)
{
    vector<bool>& x = particle->x;
    x = vector<bool>(dimension);
    int num = rand() % max_num_server + 1;    // 生成[1, max_num_server]的随机整数，表示服务器的数量
    for (int i = 0; i < num; ++i)
        x[i] = true;
    std::random_shuffle(x.begin(), x.end());

    particle->v0 = vector<double>(dimension);
    particle->v1 = vector<double>(dimension);
    particle->v  = vector<double>(dimension);
    particle->pbest = vector<bool>(dimension);
    particle->num = num;
}

void ParticleSwarmOpt::pso_search()
{
    int cnt = 0;
    int last_fitness = INF;
    for (int t = 0; t < max_t; ++t)
    {
        evaluate_swarm();
        if (gbest_fitness == last_fitness)
        {
            cnt++;
            if (cnt > 30) break;
        }
        else
        {
            cnt = 0;
            last_fitness = gbest_fitness;
        }
        update_swarm_velocity();
        update_swarm_position();
        //cout << "min cost found: " << this->gbest_fitness << endl;
    }
}

void ParticleSwarmOpt::evaluate_swarm()
{
    for (int i = 0; i < population.size(); ++i)
    {
        evaluate(population[i]);
        update_best(population[i]);
    }
}

void ParticleSwarmOpt::evaluate(Particle *particle)
{
    mcf->init(particle->x);
    particle->feasible = mcf->compute();
    if (particle->feasible)
        particle->fitness = mcf->min_cost + graph->net_fee * particle->num;
    else
        particle->fitness = INF;
}

void ParticleSwarmOpt::update_best(Particle *particle)
{
    // 更新个体历史最优解
    if (particle->fitness < particle->pbest_fitness)
    {
        particle->pbest = particle->x;
        particle->pbest_fitness = particle->fitness;
    }
    // 更新群体历史最优解
    if (particle->fitness < gbest_fitness)
    {
        gbest = particle->x;
        gbest_fitness = particle->fitness;
    }
}

void ParticleSwarmOpt::update_swarm_velocity()
{
    for (int i = 0; i < population.size(); ++i)
    {
        update_velocity(population[i]);
    }
}

void ParticleSwarmOpt::update_velocity(Particle* particle)
{
    vector<double> &v1 = particle->v1;
    vector<double> &v0 = particle->v0;
    vector<double> &v = particle->v;
    for (int j = 0; j < dimension; ++j)
    {
        // 更新v0和v1（整个PSO算法最核心的部分）
        double d1_ibest, d0_ibest, d1_gbest, d0_gbest;
        double r1 = ( (double) rand() / (double) RAND_MAX );
        double r2 = ( (double) rand() / (double) RAND_MAX );
        if (particle->pbest[j])
        {
            d1_ibest = this->c1 * r1;
            d0_ibest = -d1_ibest;
        }
        else
        {
            d1_ibest = -this->c1 * r1;
            d0_ibest = -d1_ibest;
        }
        if (gbest[j])
        {
            d1_gbest = this->c2 * r2;
            d0_gbest = -d1_gbest;
        }
        else
        {
            d1_gbest = -this->c2 * r2;
            d0_gbest = -d1_gbest;
        }
        v1[j] = this->w * v1[j] + d1_ibest + d1_gbest;
        v0[j] = this->w * v0[j] + d0_ibest + d0_gbest;

        // 更新融合速度v
        if (particle->x[j])
            v[j] = v0[j];
        else
            v[j] = v1[j];

        // 使用sigmoid函数将速度正则化到[0,1]区间
        v[j] = sigmoid(v[j]);
    }

}

void ParticleSwarmOpt::update_swarm_position()
{
    for (size_t i = 0; i < population.size(); ++i)
    {
        update_position(population[i]);
    }
}

void ParticleSwarmOpt::update_position(Particle* particle)
{
    for (size_t j = 0; j < dimension; ++j)
    {
        double r = ( (double) rand() / (double) RAND_MAX );
        if (r < particle->v[j])
        {
            particle->x[j] = !particle->x[j];
            if (particle->x[j])
                particle->num++;
            else
                particle->num--;
        }
    }
}

double ParticleSwarmOpt::sigmoid(double v)
{
    return 1 / (1 + exp(-v));
}

#endif // PSO_H_INCLUDED
