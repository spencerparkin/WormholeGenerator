#pragma once

#include "HappyMath/Vector3.h"
#include "HappyMath/AxisAlignedBoundingBox.h"
#include <memory>
#include <vector>

namespace WormholeGenerator
{
	/**
	 * 
	 */
	struct SurfacePoint
	{
		HappyMath::Vector3 location;
		HappyMath::Vector3 normal;
	};

	/**
	 * This is a spacial-partitioning data-structure designed to store points,
	 * and answer questions quickly about those points.
	 */
	class PointCloud
	{
	public:
		PointCloud();
		virtual ~PointCloud();

		/**
		 * Remove all points from this cloud, making it empty, and then reconfigure it.
		 * 
		 * @param[in] boundingBox The entire cloud must fit into this box.
		 * @param[in] minBoxVolume Nodes within the box tree can get no smaller than this.
		 */
		void Reset(const HappyMath::AxisAlignedBoundingBox& boundingBox, double minBoxVolume);

		/**
		 * Insert the given point into the cloud.
		 * 
		 * @return False is returned here if the given point is outside the cloud bounds.
		 */
		bool AddPoint(const SurfacePoint& point);

		/**
		 * Quickly find all points in the cloud within the given sphere.
		 * 
		 * @param[in] center This is the center of the sphere.
		 * @param[in] radius This is the radius of the sphere.
		 * @param[out] surfacePointArray All found points are returned in this array.
		 */
		void FindPointsInSphere(const HappyMath::Vector3& center, double radius, std::vector<SurfacePoint>& surfacePointArray) const;

		/**
		 * Quickly find the point in this cloud that is closes to the given location.
		 * If there is more than one such possible point, we just return the first we find.
		 * 
		 * @param[in] location A point in the cloud is searched for that is closed to this.
		 * @param[out] surfacePoint This will store the found point, if any.
		 * @return True is returned if a point was found.  False is returned otherwise.
		 */
		bool FindClosestPoint(const HappyMath::Vector3& location, SurfacePoint& surfacePoint) const;

	private:

		/**
		 * 
		 */
		class Node
		{
		public:
			Node();
			virtual ~Node();

			bool AddPoint(const SurfacePoint& point, double minBoxVolume);
			void FindPointsInSphere(const HappyMath::Vector3& center, double radius, std::vector<SurfacePoint>& surfacePointArray) const;
			bool FindClosestPoint(const HappyMath::Vector3& location, SurfacePoint& surfacePoint) const;

			std::vector<std::shared_ptr<Node>> childNodeArray;
			std::vector<SurfacePoint> surfacePointArray;
			HappyMath::AxisAlignedBoundingBox boundingBox;
		};

		std::shared_ptr<Node> rootNode;
		double minBoxVolume;
	};
}