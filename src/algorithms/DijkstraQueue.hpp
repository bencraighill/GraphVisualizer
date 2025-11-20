#pragma once

#include "../algorithm.hpp"
#include "../memory.hpp"
#include <limits>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>

class DijkstraQueue : public Algorithm {
public:
    inline AlgorithmType GetName() const override { return AlgorithmType::DijkstraQueue; }

    void FindPath(const AdjacencyMatrix &graph, int start, int end) override {
        start_mem();
        int n = graph.size();

        std::vector<float> dist(n, std::numeric_limits<float>::max());
        std::vector<int> parent(n, -1);
        std::vector<int> parent_edge(n, -1);

        using Element = std::pair<float, int>;
        std::priority_queue<Element, std::vector<Element>, std::greater<Element>> pq;

        dist[start] = 0.0f;
        pq.push({0.0f, start});

        while (!pq.empty()) {
            float d = pq.top().first;
            int u = pq.top().second;
            pq.pop();

            if (d > dist[u]) { continue; }

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

            for (int v = 0; v < n; ++v) {
                const auto [weight, edge_index] = graph[u][v];

                if (weight > 0.0f) {
                    float alt = dist[u] + weight;

                    if (alt < dist[v]) {
                        dist[v] = alt;
                        parent[v] = u;
                        parent_edge[v] = edge_index;

                        pq.push({alt, v});

                        m_Result.TraversedEdges.push_back(edge_index);
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
