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

	// TODO: I reaaaaly need to isolate Windows.h
	// as these leaking macros are annoying!
	struct DrawState
	{
		std::span<const Node> myTree;
		std::vector<uint16_t> myRowsPerColumn;
		std::vector<bool> myIsPartOfHoveredSubTree; // Excluding hovered node!
		std::span<const Node>::iterator myHoveredIter;
		uint16_t myMaxRows = 0;
	};
public:
	static void Draw(bool& aIsOpen);

private:
	static std::vector<Node> GenerateTree(const TaskMap& aTasks);
	static DrawState SplitTree(std::span<Node> aTree);
	
	static void DrawTree(const DrawState& aState);
	static void DrawConnections(const Node& aNode, const DrawState& aState);
	static void DrawConnection(const Node& aNode, const Node& aNextNode, const DrawState& aState);
	static void DrawNode(const Node& aNode, const DrawState& aState);
	
	static bool IsHovered(const Node& aNode, const DrawState& aState);
};