#pragma once

class GameTask;

class GameTasksDialog
{
	// Forward declaring these to avoid including GameTaskManager
	using Type = uint16_t;
	using Index = Type; // to make semantincs easier
	using TaskMap = std::unordered_map<Type, GameTask>;

	struct Node
	{
		std::vector<Index> myChildrenInd;
		Index myColumn = 0, myRow = 0;
		Type myType;

		Node(Type aType) : myType(aType) {}
	};
public:
	static void Draw(bool& aIsOpen);

private:
	static std::vector<Node> GenerateTree(const TaskMap& aTasks);
	static void SplitTree(std::span<Node> aTree);
	
	static void DrawTree(std::span<const Node> aTree);
	static void DrawConnections(const Node& aNode, std::span<const Node> aTree);
	static void DrawNode(const Node& aNode);
	
	static bool IsHovered(const Node& aNode);
};