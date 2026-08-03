#include "WormholeGenerator/WormholeCurve.h"
#include <unordered_set>
#include <math.h>

using namespace WormholeGenerator;

//--------------------------------- WormholeCurve ---------------------------------

WormholeCurve::WormholeCurve()
{
}

/*virtual*/ WormholeCurve::~WormholeCurve()
{
}

void WormholeCurve::Clear()
{
	this->rootNode.reset();
}

bool WormholeCurve::Generate(const GeneratorConfig& config)
{
	if (!config.random)
		return false;

	this->Clear();

	this->rootNode = std::make_shared<Node>();
	this->rootNode->tangentPoint = config.initialTangentPoint;

	this->GenerateRecursive(config, this->rootNode, 0);

	return true;
}

void WormholeCurve::GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth)
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

/*static*/ void WormholeCurve::GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray)
{
	controlPointArray[0] = tangentPointA.location;
	controlPointArray[3] = tangentPointB.location;

	double length = (controlPointArray[3] - controlPointArray[0]).Length() / 4.0;

	controlPointArray[1] = controlPointArray[0] + tangentPointA.unitDirection * length;
	controlPointArray[2] = controlPointArray[3] - tangentPointB.unitDirection * length;
}

/*static*/ void WormholeCurve::EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint)
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

/*static*/ void WormholeCurve::EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double t2 = t * t;
	double omt = 1.0 - t;
	double omt2 = omt * omt;

	curveDerivative = 3.0 * omt2 * (point[1] - point[0]) + 6.0 * omt * t * (point[2] - point[1]) + 3.0 * t2 * (point[3] - point[2]);
}

void WormholeCurve::ForEachRenderLine(double curveLengthPerLine, std::function<void(const HappyMath::LineSegment&)> renderFunc) const
{
	std::unordered_set<std::shared_ptr<Traveler>> travelerSet;

	for (int i = 0; i < (int)this->rootNode->childNodeArray.size(); i++)
	{
		std::shared_ptr<Traveler> traveler = std::make_shared<Traveler>();
		traveler->node = this->rootNode;
		traveler->childTarget = i;

		travelerSet.insert(traveler);
	}

	while (travelerSet.size() > 0)
	{
		std::unordered_set<std::shared_ptr<Traveler>> deadTravelersSet;

		for (std::shared_ptr<Traveler> traveler : travelerSet)
		{
			HappyMath::LineSegment lineSegment;

			traveler->CalcLocation(lineSegment.point[0]);

			std::shared_ptr<Traveler> travelerCopy = std::make_shared<Traveler>(*traveler);

			bool crossedNodeBoundary = false;
			bool advanced = traveler->Advance(curveLengthPerLine, [&crossedNodeBoundary](const Node* node) -> int
				{
					crossedNodeBoundary = true;
					return 0;
				});

			if (!advanced)
			{
				deadTravelersSet.insert(traveler);
				continue;
			}
			
			traveler->CalcLocation(lineSegment.point[1]);
			renderFunc(lineSegment);

			if (!crossedNodeBoundary)
				continue;
			
			std::shared_ptr<Node> nextNode = travelerCopy->node->childNodeArray[traveler->childTarget];
			for (int i = 1; i < (int)nextNode->childNodeArray.size(); i++)
			{
				std::shared_ptr<Traveler> newTraveler = std::make_shared<Traveler>(*travelerCopy);

				advanced = newTraveler->Advance(curveLengthPerLine, [i](const Node* node) -> int
					{
						return i;
					});

				if (advanced)
				{
					newTraveler->CalcLocation(lineSegment.point[1]);
					renderFunc(lineSegment);
					travelerSet.insert(newTraveler);
				}
			}
		}

		for (std::shared_ptr<Traveler> deadTraveler : deadTravelersSet)
			travelerSet.erase(deadTraveler);
	}
}

void WormholeCurve::ForEachNode(std::function<void(const Node*)> nodeFunc) const
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

//--------------------------------- WormholeCurve::GeneratorConfig ---------------------------------

WormholeCurve::GeneratorConfig::GeneratorConfig()
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

//--------------------------------- WormholeCurve::Traveler ---------------------------------

WormholeCurve::Traveler::Traveler()
{
	this->curveParameter = 0.0;
	this->childTarget = 0;
}

WormholeCurve::Traveler::Traveler(const Traveler& traveler)
{
	this->curveParameter = traveler.curveParameter;
	this->childTarget = traveler.childTarget;
	this->node = traveler.node;
}

/*virtual*/ WormholeCurve::Traveler::~Traveler()
{
}

bool WormholeCurve::Traveler::Advance(double curveDistance, std::function<int(const Node*)> branchPredicate, double curveParameterDelta /*= 0.1*/)
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

bool WormholeCurve::Traveler::CalcLocation(HappyMath::Vector3& curveLocation) const
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

//--------------------------------- WormholeCurve::Node ---------------------------------

WormholeCurve::Node::Node()
{
}

/*virtual*/ WormholeCurve::Node::~Node()
{
}