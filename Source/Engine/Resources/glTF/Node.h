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

		static std::vector<Node> Parse(const nlohmann::json& aRootJson)
		{
			std::vector<Node> nodes;
			const nlohmann::json& nodesJson = aRootJson["nodes"];
			nodes.reserve(nodesJson.size());

			std::unordered_map<int, int> childParentMap;

			for (const nlohmann::json& nodeJson : nodesJson)
			{
				// everything in a node is optional
				Node node;
				node.myCamera = ReadOptional(nodeJson, "camera", kInvalidInd);
				node.mySkin = ReadOptional(nodeJson, "skin", kInvalidInd);
				node.myMesh = ReadOptional(nodeJson, "mesh", kInvalidInd);

				{
					// transform can be represented as a 16-float...
					const auto& matrixJson = nodeJson.find("matrix");
					if (matrixJson != nodeJson.end())
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
#if GLM_VERSION < 999
						// https://github.com/g-truc/glm/pull/1012
						rot = glm::conjugate(rot);
#else
#error remove the hack!
#endif
						node.myTransform.SetPos(pos);
						node.myTransform.SetRotation(rot);
						node.myTransform.SetScale(scale);
					}
					else // ... or any combination of TRS
					{
						auto ExtractFunc = [&](auto aName, auto& aData)
						{
							const auto& iterJson = nodeJson.find(aName);
							if (iterJson != nodeJson.end())
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
							node.myTransform.SetRotation(rot);
						}

						glm::vec3 scale;
						if (ExtractFunc("scale", scale))
						{
							node.myTransform.SetScale(scale);
						}

						glm::vec3 pos;
						if (ExtractFunc("translation", pos))
						{
							node.myTransform.SetPos(pos);
						}
					}
				}

				node.myWorldTransform = node.myTransform;
				node.myChildren = ReadOptional(nodeJson, "children", std::vector<int>{});
				for (int child : node.myChildren)
				{
					childParentMap.emplace(child, static_cast<int>(nodes.size()));
				}
				node.myWeights = ReadOptional(nodeJson, "weights", std::vector<int>{});
				node.myName = ReadOptional(nodeJson, "name", std::string{});
				
				nodes.push_back(std::move(node));
			}

			std::queue<int> dirtyHierarchy;
			for (int i = 0; i < nodes.size(); i++)
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

			while (!dirtyHierarchy.empty())
			{
				int parentInd = dirtyHierarchy.front();
				dirtyHierarchy.pop();
				const Node& parentNode = nodes[parentInd];
				for (int childInd : parentNode.myChildren)
				{
					Node& child = nodes[childInd];
					child.myWorldTransform = parentNode.myWorldTransform * child.myTransform;
					dirtyHierarchy.push(childInd);
				}
			}
			return nodes;
		}
	};
}