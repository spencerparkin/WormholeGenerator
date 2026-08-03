#include "WormholeGenerator/WormholeTree.h"
#include <unordered_set>
#include <math.h>

using namespace WormholeGenerator;

//--------------------------------- WormholeTree ---------------------------------

WormholeTree::WormholeTree()
{
}

/*virtual*/ WormholeTree::~WormholeTree()
{
}

void WormholeTree::Clear()
{
	this->rootNode.reset();
}

bool WormholeTree::Generate(const GeneratorConfig& config)
{
	if (!config.random)
		return false;

	this->Clear();

	this->rootNode = std::make_shared<Node>();
	this->rootNode->tangentPoint = config.initialTangentPoint;

	this->GenerateRecursive(config, this->rootNode, 0);

	return true;
}

void WormholeTree::GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth)
{
	if (currentDepth > config.maxDepth)
		return;

	int branchFactor = 1;

	if (config.maxBranchFactor > 1 && config.random->InRange(0.0, 1.0) < config.branchProbability)
	{
		branchFactor = config.random->InRange(2, config.maxBranchFactor);
	}

	for (int i = 0; i < branchFactor; i++)
	{
		std::shared_ptr<Node> childNode = std::make_shared<Node>();

		double distance = config.random->InRange(config.minDistBetweenNodes, config.maxDistBetweenNodes);

		HappyMath::Vector3 unitDirection;
		unitDirection.SetAsRandomDirectionInCone(*config.random, parentNode->tangentPoint.unitDirection, config.maxAngleDeviation);

		childNode->tangentPoint.location = parentNode->tangentPoint.location + unitDirection * distance;
		childNode->tangentPoint.unitDirection.SetAsRandomDirectionInCone(*config.random, unitDirection, config.maxAngleDeviation);

		parentNode->childNodeArray.push_back(childNode);

		this->GenerateRecursive(config, childNode, currentDepth + 1);
	}
}

/*static*/ void WormholeTree::GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray)
{
	controlPointArray[0] = tangentPointA.location;
	controlPointArray[3] = tangentPointB.location;

	double length = (controlPointArray[3] - controlPointArray[0]).Length() / 2.0;

	controlPointArray[1] = controlPointArray[0] + tangentPointA.unitDirection * length;
	controlPointArray[2] = controlPointArray[3] - tangentPointB.unitDirection * length;
}

/*static*/ void WormholeTree::EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double t2 = t * t;
	double t3 = t2 * t;
	double omt = 1.0 - t;
	double omt2 = omt * omt;
	double omt3 = omt2 * omt;

	curvePoint = omt3 * point[0] + 3.0 * omt2 * t * point[1] + 3.0 * omt * t2 * point[2] + t3 * point[3];
}

/*static*/ void WormholeTree::EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double t2 = t * t;
	double omt = 1.0 - t;
	double omt2 = omt * omt;

	curveDerivative = 3.0 * omt2 * (point[1] - point[0]) + 6.0 * omt * t * (point[2] - point[1]) + 3.0 * t2 * (point[3] - point[2]);
}

void WormholeTree::ForEachRenderLine(int linesPerCurve, std::function<void(const HappyMath::LineSegment&)> renderFunc) const
{
	this->ForEachNode([linesPerCurve, renderFunc](const Node* node) -> void
		{
			const TangentPoint& tangentPointA = node->tangentPoint;

			for (int i = 0; i < (int)node->childNodeArray.size(); i++)
			{
				const TangentPoint& tangentPointB = node->childNodeArray[i]->tangentPoint;

				HappyMath::LineSegment line;
				int k = 0;

				for (int j = 0; j <= linesPerCurve; j++)
				{
					double curveParameter = double(j) / double(linesPerCurve);

					EvaluateCubicBezierCurve(tangentPointA, tangentPointB, curveParameter, line.point[k]);

					k = 1 - k;

					if (j > 0)
						renderFunc(line);
				}
			}
		});
}

void WormholeTree::ForEachNode(std::function<void(const Node*)> nodeFunc) const
{
	if (!this->rootNode.get())
		return;

	std::vector<std::shared_ptr<Node>> nodeArray;
	nodeArray.push_back(this->rootNode);

	while (nodeArray.size() > 0)
	{
		std::shared_ptr<Node> node = nodeArray[0];

		if (nodeArray.size() > 1)
			nodeArray[0] = nodeArray[nodeArray.size() - 1];

		nodeArray.pop_back();

		nodeFunc(node.get());

		for (int i = 0; i < (int)node->childNodeArray.size(); i++)
			nodeArray.push_back(node->childNodeArray[i]);
	}
}

//--------------------------------- WormholeTree::GeneratorConfig ---------------------------------

WormholeTree::GeneratorConfig::GeneratorConfig()
{
	this->random = nullptr;
	this->maxDepth = 32;
	this->maxAngleDeviation = M_PI / 12.0;
	this->initialTangentPoint.location.SetComponents(0.0, 0.0, 0.0);
	this->initialTangentPoint.unitDirection.SetComponents(0.0, 0.0, -1.0);
	this->branchProbability = 0.1;
	this->maxBranchFactor = 2;
	this->minDistBetweenNodes = 10.0;
	this->maxDistBetweenNodes = 15.0;
}

//--------------------------------- WormholeTree::Traveler ---------------------------------

WormholeTree::Traveler::Traveler()
{
	this->curveParameter = 0.0;
	this->childTarget = 0;
}

WormholeTree::Traveler::Traveler(const Traveler& traveler)
{
	this->curveParameter = traveler.curveParameter;
	this->childTarget = traveler.childTarget;
	this->node = traveler.node;
}

/*virtual*/ WormholeTree::Traveler::~Traveler()
{
}

bool WormholeTree::Traveler::Advance(double curveDistance, std::function<int(const Node*)> branchPredicate, double curveParameterDelta /*= 0.1*/)
{
	if (!this->node.get())
		return false;

	if (this->childTarget < 0 || this->childTarget >= (int)this->node->childNodeArray.size())
		return false;

	const TangentPoint& tangentPointA = this->node->tangentPoint;
	const TangentPoint& tangentPointB = this->node->childNodeArray[this->childTarget]->tangentPoint;

	double totalDistanceTraveled = 0.0;

	while (totalDistanceTraveled <= curveDistance)
	{
		HappyMath::Vector3 curveDerivative;
		EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, this->curveParameter, curveDerivative);

		HappyMath::Vector3 curveDelta = curveDerivative * curveParameterDelta;
		totalDistanceTraveled += curveDelta.Length();

		if(this->curveParameter + curveParameterDelta >= 1.0)
		{
			this->node = this->node->childNodeArray[this->childTarget];
			this->childTarget = branchPredicate(this->node.get());
		}

		this->curveParameter += curveParameterDelta;
		if (this->curveParameter > 1.0)
			this->curveParameter -= 1.0;
	}

	return true;
}

bool WormholeTree::Traveler::CalcLocation(HappyMath::Vector3& curveLocation) const
{
	if (!this->node.get())
		return false;

	if (this->childTarget < 0 || this->childTarget >= (int)this->node->childNodeArray.size())
		return false;

	const TangentPoint& tangentPointA = this->node->tangentPoint;
	const TangentPoint& tangentPointB = this->node->childNodeArray[this->childTarget]->tangentPoint;

	EvaluateCubicBezierCurve(tangentPointA, tangentPointB, this->curveParameter, curveLocation);

	return true;
}

//--------------------------------- WormholeTree::Node ---------------------------------

WormholeTree::Node::Node()
{
}

/*virtual*/ WormholeTree::Node::~Node()
{
}