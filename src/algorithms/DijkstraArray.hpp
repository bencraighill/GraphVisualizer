#pragma once

#include "../algorithm.hpp"
#include "../memory.hpp"
#include <vector>
#include <limits>
#include <algorithm>

class DijkstraArray : public Algorithm {
public:

    inline AlgorithmType GetName() const override { return AlgorithmType::DijkstraArray; }

    void FindPath(const AdjacencyMatrix& graph, int start, int end) override {
        start_mem();
        int n = graph.size();

        std::vector<float> dist(n, std::numeric_limits<float>::max());
        std::vector<bool> visited(n, false);
        std::vector<int> parent(n, -1);
        std::vector<int> parent_edge(n, -1);

        dist[start] = 0.0f;

        for (int i = 0; i < n; i++) {

            int u = -1;
            float minVal = std::numeric_limits<float>::max();

            for (int j = 0; j < n; j++) {
                if (!visited[j] && dist[j] < minVal) {
                    minVal = dist[j];
                    u = j;
                }
            }

            if (u == -1 || dist[u] == std::numeric_limits<float>::max()) {
                break;
            }

            visited[u] = true;

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
                if (weight > 0.0f && !visited[v]) {
                    float alt = dist[u] + weight;

                    if (alt < dist[v]) {
                        dist[v] = alt;
                        parent[v] = u;
                        parent_edge[v] = edge_index;
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
