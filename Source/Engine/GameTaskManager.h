#pragma once

class GameTask
{
public:
	enum Type
	{
		/* vvv RESERVED vvv */
		Uninitialised,
		GraphBroadcast, // special reserved type for kicking off the graph execution
		/* ^^^ RESERVED ^^^ */
		UpdateAssetTracker,
		AddGameObjects,
		EditorUpdate,
		UpdateInput,
		GameUpdate,
		Render,
		PhysicsUpdate,
		UpdateAudio,
		UpdateEnd,
		RemoveGameObjects
	};

public:
	GameTask();
	GameTask(Type aType, std::function<void()> aCallback);
	void operator()(tbb::flow::continue_msg) const { myCallback(); }

	void AddDependency(Type aType) { myDependencies.push_back(aType); }
	size_t GetDependencyCount() const { return myDependencies.size(); }
	Type GetDependency(size_t anIndex) const { return myDependencies[anIndex]; }

private:
	friend class GameTaskManager;
	Type GetType() const { return myType; }

	std::function<void()> myCallback;
	Type myType;
	std::vector<Type> myDependencies;
};

// thanks https://software.intel.com/en-us/node/506218
class GameTaskManager
{
public:
	GameTaskManager();
	~GameTaskManager();

	void AddTask(const GameTask& aTask);
	void ResolveDependencies();
	void Run();
	void Reset();

private:
	std::unordered_map<GameTask::Type, GameTask> myTasks;

	typedef tbb::flow::graph_node BaseTaskNodeType;
	typedef tbb::flow::broadcast_node<tbb::flow::continue_msg> StartNodeType;
	typedef tbb::flow::continue_node<tbb::flow::continue_msg> TaskNodeType;
	std::unordered_map<GameTask::Type, std::shared_ptr<BaseTaskNodeType>> myTaskNodes;
	std::shared_ptr<tbb::flow::graph> myTaskGraph;
};

