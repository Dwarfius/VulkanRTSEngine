#pragma once

#include "Common.h"

#include <Core/Transform.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace glTF
{
	struct Node
	{
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

			for (const nlohmann::json& nodeJson : nodesJson)
			{
				// everything in a node is optional
				Node node;
				node.myCamera = ReadOptional(nodeJson, "camera", kInvalidInd);
				node.mySkin = ReadOptional(nodeJson, "skin", kInvalidInd);
				node.myMesh = ReadOptional(nodeJson, "mesh", kInvalidInd);

				// TODO: adapt the glTF right handed coord system to my left handed
				{
					// transform can be represented as a 16-float...
					const auto& matrixJson = nodeJson.find("matrix");
					if (matrixJson != nodeJson.end())
					{
						glm::mat4 matrix;
						for (char i = 0; i < 16; i++)
						{
							glm::value_ptr(matrix)[i] = matrixJson[i].get<float>();
						}
						glm::vec3 scale;
						glm::quat rot;
						glm::vec3 pos;
						glm::vec3 skew;
						glm::vec4 perspective;
						if (glm::decompose(matrix, scale, rot, pos, skew, perspective))
						{
							rot = glm::conjugate(rot);
							node.myTransform.SetPos(pos);
							node.myTransform.SetRotation(rot);
							node.myTransform.SetScale(scale);
						}
					}
					else // ... or any combination of TRS
					{
						auto ExtractFunc = [&](auto aName, auto& aData)
						{
							const auto& iterJson = nodeJson.find(aName);
							if (iterJson != nodeJson.end())
							{
								for (int i = 0; i < aData.length(); i++)
								{
									glm::value_ptr(aData)[i] = iterJson.value()[i].get<float>();
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

				node.myChildren = ReadOptional(nodeJson, "children", std::vector<int>{});
				node.myWeights = ReadOptional(nodeJson, "weights", std::vector<int>{});
				node.myName = ReadOptional(nodeJson, "name", std::string{});
				nodes.push_back(std::move(node));
			}
			return nodes;
		}
	};
}