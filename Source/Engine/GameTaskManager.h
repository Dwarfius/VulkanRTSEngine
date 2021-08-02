#pragma once

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

private:
	friend class GameTaskManager;
	uint16_t GetType() const { return myType; }

	std::function<void()> myCallback;
	Type myType;
	std::vector<Type> myDependencies;
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

	void Run();
	void Wait();

private:
	void ResolveDependencies();

	// It appears first in decl to be destroyed last
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