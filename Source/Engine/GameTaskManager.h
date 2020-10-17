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
		AddGameObjects,
		EditorUpdate,
		UpdateInput,
		GameUpdate,
		Render,
		PhysicsUpdate,
		AnimationUpdate,
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
	using BaseTaskNodeType = tbb::flow::graph_node;
	using StartNodeType = tbb::flow::broadcast_node<tbb::flow::continue_msg>;
	using TaskNodeType = tbb::flow::continue_node<tbb::flow::continue_msg>;
	class DummySender;

public:
	// RAII style wrapper to add an extra dependency
	// to one of the tasks, and notify when it's done
	// on destruction
	class ExternalDependencyScope
	{
	public:
		ExternalDependencyScope(TaskNodeType& aTaskNode);
		~ExternalDependencyScope();

	private:
		TaskNodeType& myTaskNode;
	};

	GameTaskManager();
	~GameTaskManager();

	void AddTask(const GameTask& aTask);
	ExternalDependencyScope AddExternalDependency(GameTask::Type aType);

	void ResolveDependencies();
	void Run();
	void Wait();
	void Reset();

private:
	std::unordered_map<GameTask::Type, GameTask> myTasks;
	std::unordered_map<GameTask::Type, std::shared_ptr<BaseTaskNodeType>> myTaskNodes;
	std::shared_ptr<tbb::flow::graph> myTaskGraph;
	std::queue<TaskNodeType*> myExternalDepsResetQueue;
	std::atomic<bool> myIsRunning;
};

class GameTaskManager::DummySender : public tbb::flow::sender<tbb::flow::continue_msg>
{
public:
	bool register_successor(successor_type&) override final { return false; };
	bool remove_successor(successor_type&) override final { return false; };
};