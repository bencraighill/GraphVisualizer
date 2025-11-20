#pragma once

#include "../algorithm.hpp"
#include <cstdio>
#include <queue>
#include <vector>
#include <algorithm>

class BFS : public Algorithm
{
public:
	inline AlgorithmType GetName() const override { return AlgorithmType::BFS; }

	void FindPath(const AdjacencyMatrix& graph, int start, int end) override
	{
		std::queue<int> queue;
		std::vector<bool> visited(graph.size(), false);

		std::vector<int> parent(graph.size(), -1);
		std::vector<int> parentEdge(graph.size(), -1);

		visited[start] = true;
		queue.push(start);

		while (!queue.empty())
		{
			int curr = queue.front();
			queue.pop();

			if (curr == end)
			{
				std::vector<int> edges;
				int node = end;

				while (parent[node] != -1) {
					edges.push_back(parentEdge[node]);
					node = parent[node];
				}

				std::reverse(edges.begin(), edges.end());
				m_Result.FinalEdges = std::move(edges);
				return;
			}

			for (size_t i = 0; i < graph.size(); i++)
			{
				const auto [weight, edge_index] = graph[curr][i];
				if (!visited[i] && weight > 0.0f)
				{
					visited[i] = true;
					parent[i] = curr;
					parentEdge[i] = edge_index;
					queue.push(i);

					m_Result.TraversedEdges.push_back(edge_index);
				}
			}
		}

		m_Result = {};
	}

	TraversalResult GetResult() override
	{
		return m_Result;
	}

private:
	TraversalResult m_Result;
};
