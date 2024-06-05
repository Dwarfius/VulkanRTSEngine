#pragma once

#include "Common.h"

#include <Core/Transform.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace glTF
{
	struct Node
	{
		Transform myWorldTransform;
		Transform myTransform; // local space
		std::vector<int> myChildren;
		std::vector<int> myWeights;
		std::string myName;
		int myCamera;
		int mySkin;
		int myMesh;

		static void ParseItem(const nlohmann::json& aNodeJson, Node& aNode)
		{
			aNode.myCamera = ReadOptional(aNodeJson, "camera", kInvalidInd);
			aNode.mySkin = ReadOptional(aNodeJson, "skin", kInvalidInd);
			aNode.myMesh = ReadOptional(aNodeJson, "mesh", kInvalidInd);

			{
				// transform can be represented as a 16-float...
				const auto& matrixJson = aNodeJson.find("matrix");
				if (matrixJson != aNodeJson.end())
				{
					glm::mat4 matrix;
					const std::vector<float>& matrixVec = (*matrixJson).get<std::vector<float>>();
					for (char i = 0; i < 16; i++)
					{
						glm::value_ptr(matrix)[i] = matrixVec[i];
					}

					glm::vec3 scale;
					glm::quat rot;
					glm::vec3 pos;
					glm::vec3 skew;
					glm::vec4 perspective;

					const bool decomposed = glm::decompose(matrix, scale, rot, pos, skew, perspective);
					ASSERT_STR(decomposed, "According to glTF spec, matrix must be decomposable!");
					aNode.myTransform.SetPos(pos);
					aNode.myTransform.SetRotation(rot);
					aNode.myTransform.SetScale(scale);
				}
				else // ... or any combination of TRS
				{
					auto ExtractFunc = [&](auto aName, auto& aData)
					{
						const auto& iterJson = aNodeJson.find(aName);
						if (iterJson != aNodeJson.end())
						{
							const std::vector<float>& dataVec = (*iterJson).get<std::vector<float>>();
							for (int i = 0; i < aData.length(); i++)
							{
								glm::value_ptr(aData)[i] = dataVec[i];
							}
							return true;
						}
						return false;
					};

					glm::quat rot;
					if (ExtractFunc("rotation", rot))
					{
						aNode.myTransform.SetRotation(rot);
					}

					glm::vec3 scale;
					if (ExtractFunc("scale", scale))
					{
						aNode.myTransform.SetScale(scale);
					}

					glm::vec3 pos;
					if (ExtractFunc("translation", pos))
					{
						aNode.myTransform.SetPos(pos);
					}
				}
			}

			aNode.myWorldTransform = aNode.myTransform;
			aNode.myChildren = ReadOptional(aNodeJson, "children", std::vector<int>{});
			aNode.myWeights = ReadOptional(aNodeJson, "weights", std::vector<int>{});
			aNode.myName = ReadOptional(aNodeJson, "name", std::string{});
		}

		static void UpdateWorldTransforms(std::vector<Node>& aNodes)
		{
			std::unordered_map<int, int> childParentMap;

			// collect the hierarchy...
			for (const Node& node : aNodes)
			{
				for (int child : node.myChildren)
				{
					childParentMap.emplace(child, static_cast<int>(aNodes.size()));
				}
			}

			// find roots to kick off the hierarchy update...
			std::queue<int> dirtyHierarchy;
			for (int i = 0; i < aNodes.size(); i++)
			{
				auto iter = childParentMap.find(i);
				if (iter != childParentMap.end())
				{
					// if we found one, means it's not a root - so skip it
					continue;
				}

				// found a root - add for processing
				dirtyHierarchy.push(i);
			}

			// update the transforms
			while (!dirtyHierarchy.empty())
			{
				int parentInd = dirtyHierarchy.front();
				dirtyHierarchy.pop();
				const Node& parentNode = aNodes[parentInd];
				for (int childInd : parentNode.myChildren)
				{
					Node& child = aNodes[childInd];
					child.myWorldTransform = parentNode.myWorldTransform * child.myTransform;
					dirtyHierarchy.push(childInd);
				}
			}
		}
	};
}