#pragma once

#include <Core/Utils.h>

class GameTask
{
public:
	enum class ReservedTypes : uint8_t
	{
		GraphBroadcast = 0, // special reserved type for kicking off the graph execution
	};

	using Type = uint16_t;

public:
	GameTask(Type aType, std::function<void()> aCallback);
	void operator()(tbb::flow::continue_msg) const { myCallback(); }

	void AddDependency(Type aType) { myDependencies.push_back(aType); }
	const std::vector<Type>& GetDependencies() const { return myDependencies; }

	void SetName(std::string_view aName) { myName = aName; }
	std::string_view GetName() const { return myName; }

private:
	friend class GameTaskManager;
	Type GetType() const { return myType; }

	std::function<void()> myCallback;
	std::vector<Type> myDependencies;
	std::string_view myName;
	Type myType;
};

// thanks https://software.intel.com/en-us/node/506218
class GameTaskManager
{
	using BaseTaskNodeType = tbb::flow::graph_node;
	using StartNodeType = tbb::flow::broadcast_node<tbb::flow::continue_msg>;
	using TaskNodeType = tbb::flow::continue_node<tbb::flow::continue_msg>;

public:
	// RAII style wrapper to add an extra dependency
	// to one of the tasks, and notify when it's done
	// on destruction
	class ExternalDependencyScope
	{
	public:
		ExternalDependencyScope(std::shared_ptr<StartNodeType> aStartNode, TaskNodeType& aTaskNode);
		~ExternalDependencyScope();

	private:
		std::weak_ptr<StartNodeType> myStartNode;
	};

	GameTaskManager();

	void AddTask(const GameTask& aTask);
	GameTask& GetTask(GameTask::Type anId);
	ExternalDependencyScope AddExternalDependency(GameTask::Type aType);

	const std::unordered_map<GameTask::Type, GameTask>& GetTasks() const { return myTasks; }

	void Run();
	void Wait();

private:
	void ResolveDependencies();

	tbb::task_arena myTaskArena;
	Utils::AffinitySetter myAffinitySetter{ myTaskArena, Utils::AffinitySetter::Priority::High };
	std::unique_ptr<tbb::flow::graph> myTaskGraph;
	std::unordered_map<GameTask::Type, GameTask> myTasks;
	std::unordered_map<GameTask::Type, std::shared_ptr<BaseTaskNodeType>> myTaskNodes;
	struct ExternalDepPair
	{
		std::shared_ptr<StartNodeType> myExternDep;
		const GameTask::Type myTaskType;
	};
	std::queue<ExternalDepPair> myExternalDepsResetQueue;
	std::atomic<bool> myIsRunning;
	std::atomic<bool> myNeedsResolve = true;
};