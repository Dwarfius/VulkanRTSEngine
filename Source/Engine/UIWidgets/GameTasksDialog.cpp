#include "Precomp.h"
#include "GameTasksDialog.h"

#include "../Game.h"
#include "../GameTaskManager.h"
#include <Core/Profiler.h>

void GameTasksDialog::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Game Tasks"), &aIsOpen)
	{
		const GameTaskManager& taskManager = Game::GetInstance()->GetTaskManager();
		const TaskMap& tasks = taskManager.GetTasks();
		// Note - this algo is rather heavy duty (5 linear passes over the tree,
		// lots of allocations). I'm sure there's a neater algorithm(or just caching).
		// But! The task tree is veeery simple right now, so I can afford it.
		// Just in case, going to track it for the future
		Profiler::ScopedMark scopeMark("GameTasksDialog::Draw");
		std::vector<Node> tree = GenerateTree(tasks);
		DrawState state = SplitTree(tree);
		DrawTree(state);
	}
	ImGui::End();
}

std::vector<GameTasksDialog::Node> GameTasksDialog::GenerateTree(const TaskMap& aTasks)
{
	static_assert(std::is_same_v<GameTasksDialog::Type, GameTask::Type>, "Update GameTaskDialog::Type!");
	std::vector<Node> tree;
	tree.reserve(aTasks.size());

	std::unordered_map<Type, Index> typeToIndMap;
	typeToIndMap.reserve(aTasks.size());

	// store the root first!
	constexpr Type kBroadcast = static_cast<Type>(GameTask::ReservedTypes::GraphBroadcast);
	tree.emplace_back(kBroadcast, "GraphBrodcast");
	typeToIndMap.insert({ kBroadcast , 0 });

	// gather all the tasks before we start resolving indices for parents and children
	{
		uint32_t taskInd = 1;
		for (const auto& [type, task] : aTasks)
		{
			ASSERT_STR(type != kBroadcast, "This is built on the assumption that GameTaskManager doesn't"
				" store the root of the graph, but if it ever changes I got to remove manual root creation!");

			tree.emplace_back(type, task.GetName());
			typeToIndMap.insert({ type, taskInd++ });
		}
	}

	// resolve the connections
	for (const auto& [type, task] : aTasks)
	{
		const Type taskIndex = typeToIndMap[type];
		Node& currTaskNode = tree[taskIndex];
		const std::vector<Type>& taskDeps = task.GetDependencies();
		if (taskDeps.empty())
		{
			// no dependencies - it'll be kicked off from the start via Broadcast
			// so add it to root's children
			tree[0].myNextInd.push_back(taskIndex);
		}
		else
		{
			// we have dependencies, so we should add ourselves as childern for them
			for (Type depType : taskDeps)
			{
				const Index depInd = typeToIndMap[depType];
				tree[depInd].myNextInd.push_back(taskIndex);
			}
		}
	}

	// TODO: It might be useful to add an NRVO utility that is compile checked
	// it'll come at expense of slightly adjusting the code both at declaration and
	// at the callsite, but if possible, would be compile-time enforced efficiency!
	// NRVO
	return tree;
}

GameTasksDialog::DrawState GameTasksDialog::SplitTree(std::span<Node> aTree)
{
	// Splits the tree horizontally by organizing tasks in columns
	// based on the dependencies of the preceeding tasks.
	// Example:
	//   Column 1: Broadcast
	//   Column 2: Children of Broadcast
	//   Column 3: Children of Children of Broadcast
	// Importantly, I need to shift/skip children that will repeated in further columns.

	// First, create a tree with duplicate children. We will be able to de-duplicate it afterwards
	// To do this, 2 vectors should be enough - one for traversal, other for building a new column
	// and then just swap and build again
	std::vector<Index> columns[2];
	columns[0].reserve(aTree.size()); // worst case scenario
	columns[1].reserve(aTree.size());

	uint8_t currColumn = 0;
	columns[currColumn].push_back(0); // start at root
	std::unordered_set<Index> alreadyAddedChildren;
	alreadyAddedChildren.reserve(aTree.size()); // worst case scenario

	std::vector<std::vector<Index>> columnsOfNodes;
	while (!columns[currColumn].empty())
	{
		const uint8_t nextColumn = 1 - currColumn;
		columnsOfNodes.push_back({});
		std::vector<Index>& column = columnsOfNodes.back();
		column.reserve(columns[currColumn].size());

		for (Index index : columns[currColumn])
		{
			const Node& currNode = aTree[index];
			// this can add duplicates, as we might encounter children on different levels
			column.push_back(index);

			for (Index childInd : currNode.myNextInd)
			{
				if (alreadyAddedChildren.contains(childInd))
				{
					continue;
				}

				alreadyAddedChildren.insert(childInd);
				columns[nextColumn].push_back(childInd);
			}
		}

		// switch over to next column processing
		columns[currColumn].clear();
		std::swap(columns[currColumn], columns[nextColumn]);
		alreadyAddedChildren.clear();
	}

	// Now, we can deduplicate
	// We can do this in an easy way - just have to go right-to-left, removing every child we already
	// recorded
	alreadyAddedChildren.clear();
	for (size_t i = 0; i < columnsOfNodes.size() - 1; i++) // -1 because Broadcast can't be duped
	{
		std::erase_if(columnsOfNodes[columnsOfNodes.size() - 1 - i], [&](Index anIndex) {
			if (alreadyAddedChildren.contains(anIndex))
			{
				return true;
			}
			alreadyAddedChildren.insert(anIndex);
			return false;
		});
	}

	// Now that everything is split and grouped, we can assign out indices and generate a drawing state
	DrawState state;
	state.myTree = aTree;
	state.myRowsPerColumn.resize(columnsOfNodes.size());

	ASSERT(columnsOfNodes.size() <= std::numeric_limits<Index>::max());
	for (size_t column = 0; column < columnsOfNodes.size(); column++)
	{
		const std::vector<Index>& nodes = columnsOfNodes[column];
		for (size_t row = 0; row < nodes.size(); row++)
		{
			const Index nodeInd = nodes[row];
			aTree[nodeInd].myRow = row;
			aTree[nodeInd].myColumn = column;
		}

		const uint16_t columnSize = static_cast<uint16_t>(nodes.size());
		state.myRowsPerColumn[column] = columnSize;
		state.myMaxRows = std::max(state.myMaxRows, columnSize);
	}

	state.myHoveredIter = std::ranges::find_if(state.myTree, [&](const Node& aNode) {
		return IsHovered(aNode, state);
	});
	state.myIsPartOfHoveredSubTree.resize(state.myTree.size());
	if (state.myHoveredIter != state.myTree.end())
	{
		std::vector<Index>& stack = columns[0]; // reusing for tree traversal
		stack.clear();
		stack.append_range(state.myHoveredIter->myNextInd);

		while (!stack.empty())
		{
			const Index currIndex = stack.back();
			stack.pop_back();

			const Node& currNode = state.myTree[currIndex];
			state.myIsPartOfHoveredSubTree[currIndex] = true;

			stack.append_range(currNode.myNextInd);
		}
	}

	// NRVO
	return state;
}

void GameTasksDialog::DrawTree(const DrawState& aState)
{
	// all lines should go under the "Nodes", so draw them first
	if (aState.myHoveredIter != aState.myTree.end())
	{
		const Node& hoveredNode = *aState.myHoveredIter;
		const Index hoveredIndex = static_cast<Index>(std::distance(aState.myTree.begin(), aState.myHoveredIter));
		DrawConnections(*aState.myHoveredIter, aState);

		auto IsPredecessor = [&](const Node& aNode) { 
			return aNode.myColumn == hoveredNode.myColumn - 1 
				&& std::ranges::contains(aNode.myNextInd, hoveredIndex); 
		};
		for (const Node& prev : aState.myTree | std::views::filter(IsPredecessor))
		{
			DrawConnection(prev, hoveredNode, aState);
		}
	}
	else
	{
		for (const Node& node : aState.myTree)
		{
			DrawConnections(node, aState);
		}
	}

	for (const Node& node : aState.myTree)
	{
		DrawNode(node, aState);
	}
}

namespace Drawing
{
	constexpr float kSize = 60;
	constexpr float kSpace = 30;

	ImVec2 CalcNodePos(uint16_t aColumn, uint16_t aRow, uint16_t aRowCount, uint16_t aMaxRows)
	{
		constexpr float kSize = 60;
		constexpr float kSpace = 30;

		const float xOffset = ImGui::GetWindowContentRegionMin().x + kSize / 2;
		const float yOffset = ImGui::GetWindowContentRegionMin().y + kSize / 2;

		const float maxHeight = aMaxRows * (kSpace + kSize);
		const float columnHeight = aRowCount * (kSpace + kSize);

		const float x = aColumn * (kSpace + kSize);
		const float y = aRow * (kSpace + kSize) + maxHeight / 2 - columnHeight / 2;
		return { xOffset + x, yOffset + y};
	}
}

void GameTasksDialog::DrawConnections(const Node& aNode, const DrawState& aState)
{
	using namespace Drawing;

	ImVec2 startPos = CalcNodePos(aNode.myColumn, aNode.myRow, aState.myRowsPerColumn[aNode.myColumn], aState.myMaxRows);
	startPos.x += ImGui::GetWindowPos().x;
	startPos.y += ImGui::GetWindowPos().y;

	// Draw connections to next tasks
	for (Index index : aNode.myNextInd)
	{
		const Node& endNode = aState.myTree[index];
		ImVec2 endPos = CalcNodePos(endNode.myColumn, endNode.myRow, aState.myRowsPerColumn[endNode.myColumn], aState.myMaxRows);
		endPos.x += ImGui::GetWindowPos().x;
		endPos.y += ImGui::GetWindowPos().y;
		ImGui::GetWindowDrawList()->AddLine(startPos, endPos, 0xFFFFFFFF);
	}
}

void GameTasksDialog::DrawConnection(const Node& aNode, const Node& aNextNode, const DrawState& aState)
{
	using namespace Drawing;

	ImVec2 startPos = CalcNodePos(aNode.myColumn, aNode.myRow, aState.myRowsPerColumn[aNode.myColumn], aState.myMaxRows);
	startPos.x += ImGui::GetWindowPos().x;
	startPos.y += ImGui::GetWindowPos().y;

	ImVec2 endPos = CalcNodePos(aNextNode.myColumn, aNextNode.myRow, aState.myRowsPerColumn[aNextNode.myColumn], aState.myMaxRows);
	endPos.x += ImGui::GetWindowPos().x;
	endPos.y += ImGui::GetWindowPos().y;

	ImGui::GetWindowDrawList()->AddLine(startPos, endPos, 0xFFFFFFFF);
}

void GameTasksDialog::DrawNode(const Node& aNode, const DrawState& aState)
{
	using namespace Drawing;

	// Imgui packs it as ABGR
	constexpr uint32_t kDone = 0xFF00FF00;
	constexpr uint32_t kInProgress = 0xFF00FFFF;
	constexpr uint32_t kTodo = 0xFF0000FF;
	
	const bool isHovering = aState.myHoveredIter != aState.myTree.end();
	if (isHovering)
	{
		const Node& hoveredNode = *aState.myHoveredIter;
		// Dangerous if aNode is not part of the tree!
		const Index nodeInd = static_cast<Index>(std::distance(std::to_address(aState.myTree.begin()), &aNode));
		const bool isDone = hoveredNode.myColumn > aNode.myColumn;
		uint32_t color = isDone ? kDone : aState.myIsPartOfHoveredSubTree[nodeInd] ? kTodo : kInProgress;
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		// I picked too bright of a yellow so text becomes hard to see
		ImGui::PushStyleColor(ImGuiCol_Text, color == kInProgress ? 0xFF000000 : 0xFFFFFFFF);
	}

	const ImVec2 center = CalcNodePos(aNode.myColumn, aNode.myRow, aState.myRowsPerColumn[aNode.myColumn], aState.myMaxRows);
	ImGui::SetCursorPosX(center.x - kSize / 2);
	ImGui::SetCursorPosY(center.y - kSize / 2);
	ImGui::SetNextItemWidth(kSize);

	char name[8];
	Utils::StringFormat(name, "{}", aNode.myType);
	ImGui::Button(name, { kSize, kSize });

	if (isHovering)
	{
		ImGui::PopStyleColor(2);
		ImGui::SetTooltip(aNode.myName.data());
	}
}

bool GameTasksDialog::IsHovered(const Node& aNode, const DrawState& aState)
{
	using namespace Drawing;

	const ImVec2 center = CalcNodePos(aNode.myColumn, aNode.myRow, aState.myRowsPerColumn[aNode.myColumn], aState.myMaxRows);
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 min{ windowPos.x + center.x - kSize / 2, windowPos.y + center.y - kSize / 2 };
	const ImVec2 max{ windowPos.x + center.x + kSize / 2, windowPos.y + center.y + kSize / 2 };

	return ImGui::IsMouseHoveringRect(min, max);
}