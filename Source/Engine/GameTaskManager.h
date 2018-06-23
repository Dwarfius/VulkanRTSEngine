#pragma once
class GameTask
{
public:
	enum Type
	{
		/* RESERVED */
		Uninitialised,
		GraphBroadcast, // special reserved type for kicking off the graph execution
		/* RESERVED */
		AddGameObjects,
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
	GameTask(Type aType, function<void()> aCallback);
	void operator()(tbb::flow::continue_msg) const { myCallback(); }

	void AddDependency(Type aType) { myDependencies.push_back(aType); }
	size_t GetDependencyCount() const { return myDependencies.size(); }
	Type GetDependency(size_t anIndex) const { return myDependencies[anIndex]; }

private:
	friend class GameTaskManager;
	Type GetType() const { return myType; }

	function<void()> myCallback;
	Type myType;
	vector<Type> myDependencies;
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
	unordered_map<GameTask::Type, GameTask> myTasks;

	typedef tbb::flow::graph_node BaseTaskNodeType;
	typedef tbb::flow::broadcast_node<tbb::flow::continue_msg> StartNodeType;
	typedef tbb::flow::continue_node<tbb::flow::continue_msg> TaskNodeType;
	unordered_map<GameTask::Type, shared_ptr<BaseTaskNodeType>> myTaskNodes;
	shared_ptr<tbb::flow::graph> myTaskGraph;
};

