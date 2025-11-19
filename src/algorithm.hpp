#pragma once

#include <vector>
#include <utility>

using AdjacencyMatrix = std::vector<std::vector<std::pair<float, int>>>;

struct TraversalResult
{
	std::vector<int> TraversedEdges;
	std::vector<int> FinalEdges;
};

enum class AlgorithmType
{
	BFS,
	DFS,

	Count,
};

class Algorithm
{
public:
	virtual ~Algorithm() = default;

	virtual AlgorithmType GetName() const = 0;

	// Everything in here will be timed
	virtual void FindPath(const AdjacencyMatrix& graph, int start, int end) = 0;

	// This can do post processing to get the final path/result without affecting the running time.
	virtual TraversalResult GetResult() = 0;

};
