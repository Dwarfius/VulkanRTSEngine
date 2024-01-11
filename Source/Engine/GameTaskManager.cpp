#include "Precomp.h"
#include "GameTaskManager.h"

#include <Core/Profiler.h>

GameTask::GameTask(Type aType, std::function<void()> aCallback)
	: myType(aType)
	, myCallback(aCallback)
{
}

GameTaskManager::ExternalDependencyScope::ExternalDependencyScope(std::shared_ptr<StartNodeType> aStartNode, TaskNodeType& aTaskNode)
	: myStartNode(aStartNode)
{
	aStartNode->register_successor(aTaskNode);
}

GameTaskManager::ExternalDependencyScope::~ExternalDependencyScope()
{
	if (std::shared_ptr<StartNodeType> startNode = myStartNode.lock())
	{
		startNode->try_put({});
	}
}

GameTaskManager::GameTaskManager()
	: myTaskGraph(std::make_unique<tbb::flow::graph>())
	, myIsRunning(false)
{
}

void GameTaskManager::AddTask(const GameTask& aTask)
{
	ASSERT_STR(!myTasks.contains(aTask.GetType()), "Task id {} is already in use!", aTask.GetType());

	myTasks.insert({ aTask.GetType(), aTask });
	myNeedsResolve = true;
}

GameTask& GameTaskManager::GetTask(GameTask::Type anId)
{
	auto iter = myTasks.find(anId);
	ASSERT_STR(iter != myTasks.end(), "Task id {} not recognized!", anId);
	return (*iter).second;
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
	myExternalDepsResetQueue.push({ startNode, aType });
	return { startNode, task };
}

void GameTaskManager::Run()
{
	if (myNeedsResolve)
	{
		ResolveDependencies();
		myNeedsResolve = false;
	}
	myIsRunning = true;
	constexpr GameTask::Type startType = static_cast<GameTask::Type>(GameTask::ReservedTypes::GraphBroadcast);
	StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[startType].get());
	from->try_put(tbb::flow::continue_msg());
}

void GameTaskManager::Wait()
{
	Profiler::ScopedMark mark("GameTaskManager::Wait");
	myTaskGraph->wait_for_all();
	while (!myExternalDepsResetQueue.empty())
	{
		auto& [externDep, taskType] = myExternalDepsResetQueue.front();
		auto iter = myTaskNodes.find(taskType);
		TaskNodeType& task = static_cast<TaskNodeType&>(*iter->second);
		externDep->remove_successor(task);
		myExternalDepsResetQueue.pop();
	}
	myIsRunning = false;
}

void GameTaskManager::ResolveDependencies()
{
	// we're about to clear the graph, so need
	// to unlink external dependencies
	{
		size_t externDepsSize = myExternalDepsResetQueue.size();
		while (externDepsSize--)
		{
			ExternalDepPair pair = myExternalDepsResetQueue.front();
			myExternalDepsResetQueue.pop();

			auto iter = myTaskNodes.find(pair.myTaskType);
			TaskNodeType& task = static_cast<TaskNodeType&>(*iter->second);
			pair.myExternDep->remove_successor(task);

			myExternalDepsResetQueue.push(std::move(pair));
		}
	}

	// throw away all current nodes from the graph
	myTaskNodes.clear();

	// construct all the graph nodes
	constexpr GameTask::Type kStartType = static_cast<GameTask::Type>(GameTask::ReservedTypes::GraphBroadcast);
	myTaskNodes[kStartType] = std::make_shared<StartNodeType>(*myTaskGraph);
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
			for (size_t i = 0; i < count; i++)
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
			StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[kStartType].get());
			TaskNodeType* to = static_cast<TaskNodeType*>(myTaskNodes[taskType].get());
			tbb::flow::make_edge(*from, *to);
		}
	}

	// lastly, we might've had external dependencies scheduled
	// we need to relink them with the new graph nodes
	{
		size_t externDepsSize = myExternalDepsResetQueue.size();
		while (externDepsSize--)
		{
			ExternalDepPair pair = myExternalDepsResetQueue.front();
			myExternalDepsResetQueue.pop();

			auto iter = myTaskNodes.find(pair.myTaskType);
			TaskNodeType& task = static_cast<TaskNodeType&>(*iter->second);
			pair.myExternDep->register_successor(task);

			myExternalDepsResetQueue.push(std::move(pair));
		}
	}
}