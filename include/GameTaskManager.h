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
		CollisionUpdate,
		UpdateAudio,
		UpdateEnd,
		RemoveGameObjects
	};

public:
	GameTask();
	GameTask(Type aType, function<void()> aCallback);
	void operator()(tbb::flow::continue_msg) const { callback(); }

	void AddDependency(Type aType) { dependencies.push_back(aType); }
	size_t GetDependencyCount() const { return dependencies.size(); }
	Type GetDependency(size_t anIndex) const { return dependencies[anIndex]; }

private:
	friend class GameTaskManager;
	Type GetType() const { return type; }

	function<void()> callback;
	Type type;
	vector<Type> dependencies;
};

// thanks https://software.intel.com/en-us/node/506218
class GameTaskManager
{
public:
	GameTaskManager();
	~GameTaskManager();

	void AddTask(const GameTask& task);
	void ResolveDependencies();
	void Run();
	void Reset();

private:
	unordered_map<GameTask::Type, GameTask> tasks;

	typedef tbb::flow::graph_node BaseTaskNodeType;
	typedef tbb::flow::broadcast_node<tbb::flow::continue_msg> StartNodeType;
	typedef tbb::flow::continue_node<tbb::flow::continue_msg> TaskNodeType;
	unordered_map<GameTask::Type, shared_ptr<BaseTaskNodeType>> taskNodes;
	shared_ptr<tbb::flow::graph> taskGraph;
};

