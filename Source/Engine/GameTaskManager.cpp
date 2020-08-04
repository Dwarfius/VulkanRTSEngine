#include "Precomp.h"
#include "GameTaskManager.h"

GameTask::GameTask()
	: myType(Uninitialised)
{
}

GameTask::GameTask(Type aType, std::function<void()> aCallback)
	: myType(aType)
	, myCallback(aCallback)
{
}

GameTaskManager::ExternalDependencyScope::ExternalDependencyScope(GameTaskManager::TaskNodeType& aTaskNode)
	: myTaskNode(aTaskNode)
{
	myTaskNode.register_predecessor(DummySender());
}

GameTaskManager::ExternalDependencyScope::~ExternalDependencyScope()
{
	myTaskNode.try_put({});
}

GameTaskManager::GameTaskManager()
	: myTaskGraph(nullptr)
	, myIsRunning(false)
{
}

GameTaskManager::~GameTaskManager()
{
	Reset();
}

void GameTaskManager::AddTask(const GameTask& aTask)
{
	myTasks[aTask.GetType()] = aTask;
}

GameTaskManager::ExternalDependencyScope GameTaskManager::AddExternalDependency(GameTask::Type aType)
{
	ASSERT_STR(aType != GameTask::Type::GraphBroadcast, 
		"Can't add dependencies for starting node of a graph!");
	ASSERT_STR(!myIsRunning, "Can't add an external dependency while"
		" the task graph is already executing! This is a race condition!");
	auto iter = myTaskNodes.find(aType);
	ASSERT_STR(iter != myTaskNodes.end(), "Failed to find a task!");
	TaskNodeType& task = static_cast<TaskNodeType&>(*iter->second);
	myExternalDepsResetQueue.push(&task);
	return { task };
}

void GameTaskManager::ResolveDependencies()
{
	myTaskGraph = std::make_shared<tbb::flow::graph>();
	
	// first construct all the graph nodes
	myTaskNodes[GameTask::Type::GraphBroadcast] = std::make_shared<StartNodeType>(*myTaskGraph);
	for (const std::pair<GameTask::Type, GameTask>& pair : myTasks)
	{
		const GameTask::Type taskType = pair.first;
		myTaskNodes[taskType] = std::make_shared<TaskNodeType>(*myTaskGraph, myTasks[taskType]);;
	}

	// then we can make the graph edges
	for (auto iter = myTasks.cbegin(); iter != myTasks.cend(); iter++)
	{
		const GameTask::Type taskType = (*iter).first;
		const GameTask& gameTask = (*iter).second;
		if (size_t count = gameTask.GetDependencyCount())
		{
			for(size_t i=0; i<count; i++)
			{
				// edge from dependent on to depending
				TaskNodeType* from = static_cast<TaskNodeType*>(myTaskNodes[gameTask.GetDependency(i)].get());
				TaskNodeType* to = static_cast<TaskNodeType*>(myTaskNodes[taskType].get());
				tbb::flow::make_edge(*from, *to);
			}
		}
		else
		{
			// with no dependencies we can kick it off at the start
			StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[GameTask::Type::GraphBroadcast].get());
			TaskNodeType* to = static_cast<TaskNodeType*>(myTaskNodes[taskType].get());
			tbb::flow::make_edge(*from, *to);
		}
	}
}

void GameTaskManager::Run()
{
	myIsRunning = true;
	StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[GameTask::Type::GraphBroadcast].get());
	from->try_put(tbb::flow::continue_msg());
}

void GameTaskManager::Wait()
{
	myTaskGraph->wait_for_all();
	while (!myExternalDepsResetQueue.empty())
	{
		TaskNodeType* taskNode = myExternalDepsResetQueue.front();
		taskNode->remove_predecessor(DummySender{});
		myExternalDepsResetQueue.pop();
	}
	myIsRunning = false;
}

void GameTaskManager::Reset()
{
	myTaskNodes.clear();
	myTasks.clear();
}