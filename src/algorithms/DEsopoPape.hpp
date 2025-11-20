#pragma once

#include "../algorithm.hpp"
#include "../memory.hpp"
#include <vector>
#include <deque>
#include <limits>
#include <algorithm>

class DEsopoPape : public Algorithm {
public:
    inline AlgorithmType GetName() const override { return AlgorithmType::DEsopoPape; }

    void FindPath(const AdjacencyMatrix& graph, int start, int end) override {
        start_mem();
        int n = graph.size();
        std::vector<int> state(n, 2);
        std::vector<float> dist(n, std::numeric_limits<float>::max());
        std::vector<int> parent(n, -1);
        std::vector<int> parent_edge(n, -1);

        std::deque<int> q;

        dist[start] = 0.0f;
        state[start] = 1;
        q.push_back(start);

        while (!q.empty()) {
            int u = q.front();
            q.pop_front();

            state[u] = 0;

            if (u == end) {
                std::vector<int> edges;
                int node = end;

                while (parent[node] != -1) {
                    edges.push_back(parent_edge[node]);
                    node = parent[node];
                }

                std::reverse(edges.begin(), edges.end());
                m_Result.FinalEdges = std::move(edges);
                m_Result.Memory = end_mem();
                return;
            }

            for (int v = 0; v < n; v++) {
                const auto [weight, edge_index] = graph[u][v];

                if (weight > 0.0f) {
                    float alt = dist[u] + weight;

                    if (alt < dist[v]) {
                        dist[v] = alt;
                        parent[v] = u;
                        parent_edge[v] = edge_index;

                        m_Result.TraversedEdges.push_back(edge_index);

                        if (state[v] == 2) {
                            state[v] = 1;
                            q.push_back(v);
                        }
                        else if (state[v] == 0) {
                            state[v] = 1;
                            q.push_front(v);
                        }

                    }
                }
            }
        }

        m_Result = {};
        m_Result.Memory = end_mem();
    }

    TraversalResult GetResult() override {
        return m_Result;
    }

private:
    TraversalResult m_Result;
};
