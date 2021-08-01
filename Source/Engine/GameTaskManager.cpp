#include "Precomp.h"
#include "GameTaskManager.h"

GameTask::GameTask(Type aType, std::function<void()> aCallback)
	: myType(aType)
	, myCallback(aCallback)
{
}

GameTaskManager::ExternalDependencyScope::ExternalDependencyScope(std::shared_ptr<StartNodeType> aStartNode, TaskNodeType& aTaskNode)
	: myStartNode(aStartNode)
	, myTaskNode(aTaskNode)
{
	aStartNode->register_successor(myTaskNode);
}

GameTaskManager::ExternalDependencyScope::~ExternalDependencyScope()
{
	if (std::shared_ptr<StartNodeType> startNode = myStartNode.lock())
	{
		startNode->try_put({});
	}
}

GameTaskManager::GameTaskManager()
	: myTaskGraph(nullptr)
	, myIsRunning(false)
{
}

void GameTaskManager::AddTask(const GameTask& aTask)
{
	ASSERT_STR(!myTasks.contains(aTask.GetType()), "Task id %hu is already in use!", aTask.GetType());

	myTasks.insert({ aTask.GetType(), aTask });
}

GameTaskManager::ExternalDependencyScope GameTaskManager::AddExternalDependency(GameTask::Type aType)
{
	ASSERT_STR(aType != static_cast<GameTask::Type>(GameTask::ReservedTypes::GraphBroadcast),
		"Can't add dependencies for starting node of a graph!");
	ASSERT_STR(!myIsRunning, "Can't add an external dependency while"
		" the task graph is already executing! This is a race condition!");

	auto iter = myTaskNodes.find(aType);
	ASSERT_STR(iter != myTaskNodes.end(), "Failed to find a task!");
	TaskNodeType& task = static_cast<TaskNodeType&>(*iter->second);

	std::shared_ptr<StartNodeType> startNode = std::make_shared<StartNodeType>(*myTaskGraph.get());
	myExternalDepsResetQueue.push({ startNode, task });
	return { startNode, task };
}

void GameTaskManager::ResolveDependencies()
{
	myTaskGraph = std::make_unique<tbb::flow::graph>();
	
	// first construct all the graph nodes
	constexpr GameTask::Type startType = static_cast<GameTask::Type>(GameTask::ReservedTypes::GraphBroadcast);
	myTaskNodes[startType] = std::make_shared<StartNodeType>(*myTaskGraph);
	for (const auto [taskType, gameTask] : myTasks)
	{
		myTaskNodes[taskType] = std::make_shared<TaskNodeType>(*myTaskGraph, gameTask);
	}

	// then we can make the graph edges
	for (const auto [taskType, gameTask] : myTasks)
	{
		const std::vector<GameTask::Type>& dependencies = gameTask.GetDependencies();
		if (size_t count = dependencies.size())
		{
			for(size_t i=0; i<count; i++)
			{
				// edge from dependent on to depending
				TaskNodeType* from = static_cast<TaskNodeType*>(myTaskNodes[dependencies[i]].get());
				TaskNodeType* to = static_cast<TaskNodeType*>(myTaskNodes[taskType].get());
				tbb::flow::make_edge(*from, *to);
			}
		}
		else
		{
			// with no dependencies we can kick it off at the start
			StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[startType].get());
			TaskNodeType* to = static_cast<TaskNodeType*>(myTaskNodes[taskType].get());
			tbb::flow::make_edge(*from, *to);
		}
	}
}

void GameTaskManager::Run()
{
	myIsRunning = true;
	constexpr GameTask::Type startType = static_cast<GameTask::Type>(GameTask::ReservedTypes::GraphBroadcast);
	StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[startType].get());
	from->try_put(tbb::flow::continue_msg());
}

void GameTaskManager::Wait()
{
	myTaskGraph->wait_for_all();
	while (!myExternalDepsResetQueue.empty())
	{
		ExternalDepPair& pair = myExternalDepsResetQueue.front();
		pair.myExternDep->remove_successor(pair.myTask);
		myExternalDepsResetQueue.pop();
	}
	myIsRunning = false;
}