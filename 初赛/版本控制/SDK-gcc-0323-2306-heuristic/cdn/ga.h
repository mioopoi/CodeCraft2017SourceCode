#ifndef GA_H_INCLUDED
#define GA_H_INCLUDED

#include <vector>
#include <cstdlib>
#include <algorithm>

#include "globals.h"
#include "graph.h"
#include "min_cost_flow.h"

using namespace std;

struct Ind
{
    vector<bool> chromosome;
    int num;
    int fitness;
    bool feasible;
    Ind(int _num = 0, int _fitness = INF, bool _feasible = false)
    {
        num = _num;
        fitness = _fitness;
        feasible = _feasible;
    }
    bool operator< (const Ind &other) const
    {
        return this->fitness < other.fitness;
    }
};

class GeneticAlgorithm
{
public:
    double pc;         // 交叉概率
    double pm;         // 变异概率
    int pop_size;      // 种群规模
    int max_gen;       // 最大进化代数
    MinCostFlow* god;  // 用于评估个体适应度
    Ind best_ind;
    Graph* graph;
    vector<Ind> population;
    GeneticAlgorithm(Graph* graph);
    void init_population();
    void evaluate(Ind& ind);                       // 评估个体适应度
    void evolve();                                 // 进化主程序
    void cross();
    void mutate();
    void keep_size();
private:
    int len_chromosome;
    int max_num_server;
    void generate_ind(Ind& ind);
    void gen_cross_pair(vector<int>& cross_pair);
    void crossover(Ind& father, Ind& mother);
    void single_mutate(Ind& ind);
};

GeneticAlgorithm::GeneticAlgorithm(Graph* graph)
{
    srand((unsigned) time(NULL));
    this->pc = 0.70;
    this->pm = 0.03;
    this->pop_size = 100;
    this->max_gen = 100;
    this->graph = graph;
    this->len_chromosome = graph->net_node_num;
    this->max_num_server = graph->cus_node_num;
    this->god = new MinCostFlow(graph);
}

void GeneticAlgorithm::evolve()
{
    init_population();
    for (int i = 0; i < this->max_gen; ++i)
    {
        //this->pm = 0.5 - 0.3 * i / max_gen;
        cross();
        mutate();
        keep_size();
    }
    best_ind = population[0];
    cout << best_ind.fitness << endl;
    cout << best_ind.feasible << endl;
}

void GeneticAlgorithm::init_population()
{
    for (int i = 0; i < pop_size; ++i)
    {
        Ind ind;
        generate_ind(ind);
        //if (ind.feasible)
        //{
            population.push_back(ind);
        //}
    }
    //for (const Ind& ind : population)
    //{
    //    cout << "fitness: " << ind.fitness << endl;
    //}
}

void GeneticAlgorithm::generate_ind(Ind& ind)
{
    vector<bool> chromosome(len_chromosome);
    int num = rand() % max_num_server + 1;    // 生成[1, max_num_server]的随机整数，表示服务器的数量
    for (int i = 0; i < num; ++i)
    {
        chromosome[i] = true;
    }
    std::random_shuffle(chromosome.begin(), chromosome.end());
    //for (int i = 0; i < chromosome.size(); ++i)
    //    cout << chromosome[i];
    //cout << endl;
    ind.num = num;
    ind.chromosome = chromosome;
    evaluate(ind);
}

void GeneticAlgorithm::evaluate(Ind& ind)
{
    god->init(ind.chromosome);
    ind.feasible = god->compute();
    if (ind.feasible)
        ind.fitness = god->min_cost + graph->net_fee * ind.num;
    else
        ind.fitness = INF;
}

void GeneticAlgorithm::cross()
{
    int cur_size = population.size();
    vector<int> cross_pair(cur_size);
    gen_cross_pair(cross_pair);
    for (int i = 0; i < cur_size; i += 2)
    {
        double p = ( (double) rand() / (double) RAND_MAX );
        if (p <= this->pc)
        {
            crossover(population[ cross_pair[i] ], population[ cross_pair[i+1] ]);
        }
    }
}

void GeneticAlgorithm::gen_cross_pair(vector<int>& cross_pair)
{
    for (int i = 0; i < cross_pair.size(); ++i)
    {
        cross_pair[i] = i;
    }
    random_shuffle(cross_pair.begin(), cross_pair.end());
}

void GeneticAlgorithm::crossover(Ind& father, Ind& mother)
{
    // 单点交叉
    int cross_point = rand() % len_chromosome;
    Ind child1 = father, child2 = mother;
    for (int i = cross_point; i < len_chromosome; ++i)
    {
        child1.chromosome[i] = mother.chromosome[i];
        if (child1.chromosome[i] != father.chromosome[i])
        {
            if (child1.chromosome[i])
                child1.num++;
            else
                child1.num--;
        }
        child2.chromosome[i] = father.chromosome[i];
        if (child2.chromosome[i] != mother.chromosome[i])
        {
            if (child2.chromosome[i])
                child2.num++;
            else
                child2.num--;
        }
    }
    evaluate(child1);
    evaluate(child2);
    population.push_back(child1);
    population.push_back(child2);
}

void GeneticAlgorithm::mutate()
{
    for (int i = 0; i < population.size(); ++i)
    {
        double p = ( (double) rand() / (double) RAND_MAX );
        if (p < this->pm)
        {
            single_mutate(population[i]);
        }
    }
}

void GeneticAlgorithm::single_mutate(Ind& ind)
{
    // 单点变异
    //ind.chromosome.flip();
    int mutate_point = rand() % len_chromosome;
    ind.chromosome[mutate_point] = !ind.chromosome[mutate_point];
    if (ind.chromosome[mutate_point])
        ind.num++;
    else
        ind.num--;
    evaluate(ind);
}

void GeneticAlgorithm::keep_size()
{
    sort(population.begin(), population.end());
    while (population.size() > pop_size)
    {
        population.pop_back();
    }
}


#endif // GA_H_INCLUDED
