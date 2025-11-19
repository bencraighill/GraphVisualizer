#pragma once

#include "../algorithm.hpp"
#include <vector>
#include <limits>

class FloydWarshall : public Algorithm {
public:
    inline AlgorithmType GetName() const override { return AlgorithmType::FloydWarshall; }

    void FindPath(const AdjacencyMatrix& graph, int start, int end) override {
        float inf = std::numeric_limits<float>::max();
        std::vector<std::vector<float>> dist(graph.size(), std::vector<float>(graph.size(), inf));
        std::vector<std::vector<int>> prev(graph.size(), std::vector<int>(graph.size(), -1));
        std::vector<std::vector<int>> look_up(graph.size(), std::vector<int>(graph.size(), -1));

        for (int u = 0; u < graph.size(); u++) {
            dist[u][u] = 0.0f;
            prev[u][u] = u;
            for (int v = 0; v < graph.size(); v++) {
                const auto [weight, edge_index] = graph[u][v];
                if (weight > 0.0f) {
                    dist[u][v] = weight;
                    prev[u][v] = v;
                    look_up[u][v] = edge_index;
                }
            }
        }

        for (int k = 0; k < graph.size(); k++) {
            for (int i = 0; i < graph.size(); i++) {
                if (dist[i][k] == inf) continue;
                for (int j = 0; j < graph.size(); j++) {
                    if (dist[k][j] == inf) continue;
                    if (dist[i][j] > dist[i][k] + dist[k][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        prev[i][j] = prev[i][k];
                    }
                }
            }
        }

        if (dist[start][end] == inf) {
            m_Result = {};
            return;
        }

        std::vector<int> edges;
        int curr = start;

        while (curr != end) {
            int next = prev[curr][end];

            if (next == -1) {
                m_Result = {};
                return;
            }

            if (look_up[curr][next] != -1) {
                edges.push_back(look_up[curr][next]);
            }

            curr = next;
        }

        m_Result.FinalEdges = std::move(edges);
    }

    TraversalResult GetResult() override {
        return m_Result;
    }

private:
    TraversalResult m_Result;
};