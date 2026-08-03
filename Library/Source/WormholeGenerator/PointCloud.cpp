#include "WormholeGenerator/PointCloud.h"
#include <assert.h>
#include <algorithm>

using namespace WormholeGenerator;

//------------------------------------------ PointCloud ------------------------------------------

PointCloud::PointCloud()
{
	this->minBoxVolume = 8.0;
}

/*virtual*/ PointCloud::~PointCloud()
{
}

void PointCloud::Reset(const HappyMath::AxisAlignedBoundingBox& boundingBox, double minBoxVolume)
{
	this->rootNode = std::make_shared<Node>();
	this->rootNode->boundingBox = boundingBox;
	this->minBoxVolume = minBoxVolume;
}

bool PointCloud::AddPoint(const SurfacePoint& point)
{
	if (!this->rootNode.get() || !this->rootNode->boundingBox.ContainsPoint(point.location))
		return false;

	return this->rootNode->AddPoint(point, this->minBoxVolume);
}

void PointCloud::FindPointsInSphere(const HappyMath::Vector3& center, double radius, std::vector<SurfacePoint>& surfacePointArray) const
{
	if (!this->rootNode.get())
		return;

	if (!this->rootNode->boundingBox.OverlapsSphere(center, radius))
		return;

	this->rootNode->FindPointsInSphere(center, radius, surfacePointArray);
}

bool PointCloud::FindClosestPoint(const HappyMath::Vector3& location, SurfacePoint& surfacePoint) const
{
	if (!this->rootNode.get())
		return false;

	return this->rootNode->FindClosestPoint(location, surfacePoint);
}

//------------------------------------------ PointCloud::Node ------------------------------------------

PointCloud::Node::Node()
{
}

/*virtual*/ PointCloud::Node::~Node()
{
}

bool PointCloud::Node::AddPoint(const SurfacePoint& point, double minBoxVolume)
{
	if (this->boundingBox.GetVolume() <= minBoxVolume)
	{
		this->surfacePointArray.push_back(point);
		return true;
	}

	if (this->childNodeArray.size() == 0)
	{
		std::shared_ptr<Node> nodeA = std::make_shared<Node>();
		std::shared_ptr<Node> nodeB = std::make_shared<Node>();

		this->boundingBox.Split(nodeA->boundingBox, nodeB->boundingBox);

		this->childNodeArray.push_back(nodeA);
		this->childNodeArray.push_back(nodeB);
	}

	// If a point lies on the boundary between two or more boxes,
	// we just put the point in the first box we find.
	for (std::shared_ptr<Node> childNode : this->childNodeArray)
	{
		if (childNode->boundingBox.ContainsPoint(point.location))
		{
			return childNode->AddPoint(point, minBoxVolume);
		}
	}

	// We should never get here!
	assert(false);
	return false;
}

void PointCloud::Node::FindPointsInSphere(const HappyMath::Vector3& center, double radius, std::vector<SurfacePoint>& surfacePointArray) const
{
	for (const SurfacePoint& surfacePoint : this->surfacePointArray)
	{
		if ((surfacePoint.location - center).SquareLength() < radius * radius)
		{
			surfacePointArray.push_back(surfacePoint);
		}
	}

	for (int i = 0; i < (int)this->childNodeArray.size(); i++)
	{
		const Node* node = this->childNodeArray[i].get();
		if (node->boundingBox.OverlapsSphere(center, radius))
		{
			node->FindPointsInSphere(center, radius, surfacePointArray);
		}
	}
}

bool PointCloud::Node::FindClosestPoint(const HappyMath::Vector3& location, SurfacePoint& surfacePoint) const
{
	struct Box
	{
		const Node* node;
		double squareDistance;
	};

	std::vector<Box> boxArray;
	boxArray.reserve(this->childNodeArray.size());

	for (int i = 0; i < (int)this->childNodeArray.size(); i++)
	{
		Box box;
		box.node = this->childNodeArray[i].get();
		box.squareDistance = box.node->boundingBox.CalcShortestSquareDistanceToPoint(location);
		boxArray.push_back(box);
	}

	std::sort(boxArray.begin(), boxArray.end(), [&location](const Box& boxA, const Box& boxB) -> int
		{
			return boxA.squareDistance < boxB.squareDistance;
		});

	double smallestSquareDistance = std::numeric_limits<double>::max();

	for (int i = 0; i < (int)boxArray.size(); i++)
	{
		const Box& box = boxArray[i];

		SurfacePoint tentativeSurfacePoint;

		// Something went wrong if we didn't find a point.
		if (!box.node->FindClosestPoint(location, tentativeSurfacePoint))
			return false;
		
		double squareDistance = (tentativeSurfacePoint.location - location).SquareLength();
			
		if (squareDistance < smallestSquareDistance)
		{
			smallestSquareDistance = squareDistance;
			surfacePoint = tentativeSurfacePoint;
		}

		// Can we early out?
		if (i + 1 < (int)boxArray.size())
		{
			const Box& nextBox = boxArray[i + 1];
			if (smallestSquareDistance < nextBox.squareDistance)
				break;
		}
	}

	for (int i = 0; i < (int)this->surfacePointArray.size(); i++)
	{
		const SurfacePoint& tentativeSurfacePoint = this->surfacePointArray[i];

		double squareDistance = (tentativeSurfacePoint.location - location).SquareLength();

		if (squareDistance < smallestSquareDistance)
		{
			smallestSquareDistance = squareDistance;
			surfacePoint = tentativeSurfacePoint;
		}
	}

	return smallestSquareDistance != std::numeric_limits<double>::max();
}