#pragma once

#include "HappyMath/Vector3.h"
#include "HappyMath/Random.h"
#include "HappyMath/LineSegment.h"
#include <memory>
#include <vector>
#include <functional>

namespace WormholeGenerator
{
	/**
	 * These are cubic Bezier curves chained together so that we get
	 * continuity of the derivative across boundaries, but we're also
	 * a tree.  This allows the curve to branch.
	 */
	class WormholeCurve
	{
	private:
		friend class Traveler;

	public:
		class Node;

		WormholeCurve();
		virtual ~WormholeCurve();

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

		class Node
		{
		public:
			Node();
			virtual ~Node();

			TangentPoint tangentPoint;
			std::vector<std::shared_ptr<Node>> childNodeArray;
		};

		/**
		 * 
		 */
		class Traveler
		{
		public:
			Traveler();
			Traveler(const Traveler& traveler);
			virtual ~Traveler();

			bool Advance(double curveDistance, std::function<int(const Node*)> branchPredicate, double curveParameterDelta = 0.1);
			bool CalcLocation(HappyMath::Vector3& curveLocation) const;

			double curveParameter;
			std::shared_ptr<Node> node;
			int childTarget;
		};

		void Clear();
		bool Generate(const GeneratorConfig& config);
		void ForEachRenderLine(double curveLengthPerLine, std::function<void(const HappyMath::LineSegment&)> renderFunc) const;
		void ForEachNode(std::function<void(const Node*)> nodeFunc) const;

	private:

		void GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth);

		static void EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint);
		static void EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative);
		static void GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray);

		std::shared_ptr<Node> rootNode;
	};
}