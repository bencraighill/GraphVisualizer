#pragma once

#include "../algorithm.hpp"
#include "../memory.hpp"
#include <vector>

static bool dfs(const AdjacencyMatrix &graph, std::vector<bool> &visited, int curr, int end, std::vector<int> &res, std::vector<int> &s, int index) {
    if (curr == end) {
        return true;
    }
    visited[curr] = true;
    res.push_back(index);
    s.push_back(index);
    for (size_t i = 0; i < graph.size(); i++) {
        if (!visited[i] && graph[curr][i].first > 0.0f) {
            if (dfs(graph, visited, i, end, res, s, graph[curr][i].second)) {
                res.push_back(graph[curr][i].second);
                s.push_back(graph[curr][i].second);
                return true;
            }
        }
    }
    s.pop_back();
    return false;
}

class DFS : public Algorithm {
public:
    void FindPath(const AdjacencyMatrix& graph, int start, int end) override {
        start_mem();
        std::vector<int> path, s;
        std::vector<bool> visited(graph.size(), false);
        dfs(graph, visited, start, end, path, s, start);
        m_Result.TraversedEdges = path;
        m_Result.FinalEdges = s;
        m_Result.Memory = end_mem();
    }

    inline AlgorithmType GetName() const override { return AlgorithmType::DFS; }

    TraversalResult GetResult() override {
		return m_Result;
	}
private:
    TraversalResult m_Result;
};
