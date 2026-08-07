#pragma once

#include "WormholeGenerator/WormholeTree.h"
#include "glad/gl.h"
#include <string>

namespace WormholeGenerator
{
	/**
	 * This class can load wormhole data and then provide an efficient way to render it using OpenGL.
	 */
	class WormholeRenderer
	{
	public:
		WormholeRenderer();
		virtual ~WormholeRenderer();

		void Clear();
		bool LoadWormholeData(const std::string& filePath);
		void Render();
		void Advance(double deltaTime);
		void SetSpeed(double speed);
		double GetSpeed() const;
		void ApplyRoll(double rollAngleDelta);

		class RenderNode : public WormholeTree::Node
		{
		public:
			RenderNode();
			virtual ~RenderNode();

			GLuint vertexBufferObject;
			GLuint indexBufferObject;
		};

	private:
		double speed;

		std::shared_ptr<RenderNode> rootNode;
	};
}