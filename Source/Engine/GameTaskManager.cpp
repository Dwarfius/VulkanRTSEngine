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

GameTaskManager::GameTaskManager()
	: myTaskGraph(nullptr)
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
	StartNodeType* from = static_cast<StartNodeType*>(myTaskNodes[GameTask::Type::GraphBroadcast].get());
	from->try_put(tbb::flow::continue_msg());
	myTaskGraph->wait_for_all();
}

void GameTaskManager::Reset()
{
	myTaskNodes.clear();
	myTasks.clear();
}