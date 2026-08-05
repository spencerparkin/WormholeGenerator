#pragma once

#include "HappyMath/Vector3.h"
#include "HappyMath/Random.h"
#include "HappyMath/LineSegment.h"
#include "HappyMath/Polygon.h"
#include "HappyMath/Graph.h"
#include <memory>
#include <vector>
#include <functional>

namespace WormholeGenerator
{
	struct SurfacePoint
	{
		HappyMath::Vector3 location;
		HappyMath::Vector3 normal;
	};

	/**
	 * These are cubic Bezier curves chained together so that we get
	 * continuity of the derivative across boundaries, but we're also
	 * a tree.  This allows the curve to branch.
	 */
	class WormholeTree
	{
	private:
		friend class Traveler;

	public:
		class Node;

		WormholeTree();
		virtual ~WormholeTree();

		struct TangentPoint
		{
			HappyMath::Vector3 location;
			HappyMath::Vector3 unitDirection;
		};

		class GeneratorConfig
		{
		public:
			GeneratorConfig();

			HappyMath::Random* random;
			int maxDepth;
			double maxAngleDeviation;
			TangentPoint initialTangentPoint;
			double branchProbability;
			int maxBranchFactor;
			double minDistBetweenNodes;
			double maxDistBetweenNodes;
		};

		class Node : public std::enable_shared_from_this<Node>
		{
		public:
			Node();
			virtual ~Node();

			TangentPoint tangentPoint;
			std::vector<std::shared_ptr<Node>> childNodeArray;
		};

		void Clear();
		bool Generate(const GeneratorConfig& config);
		void ForEachRenderLine(int linesPerCurve, std::function<void(const HappyMath::LineSegment&)> renderFunc) const;
		void ForEachNode(std::function<void(const Node*)> nodeFunc) const;
		void GenerateSurfacePoints(std::function<void(const SurfacePoint&)> pointFunc) const;
		void PopulateGraphWithSurfacePoints(HappyMath::Graph& graph) const;

	private:

		void GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth);
		void GenerateSurfacePointsForNode(const Node* node, std::function<void(const SurfacePoint&)> pointFunc) const;
		
		static void EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint);
		static void EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative);
		static void EvaluateCubicBezierCurveSecondDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& secondCurveDerivative);
		static void GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray);
		static void FindClosestPointOnCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, const HappyMath::Vector3& point, HappyMath::Vector3& closestPoint);

		std::shared_ptr<Node> rootNode;
	};
}