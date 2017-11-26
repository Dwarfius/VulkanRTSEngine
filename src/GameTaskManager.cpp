#include "Common.h"
#include "GameTaskManager.h"

GameTask::GameTask()
	: type(Uninitialised)
{
}

GameTask::GameTask(Type aType, function<void()> aCallback)
	: type(aType)
	, callback(aCallback)
{
}

GameTaskManager::GameTaskManager()
	: taskGraph(nullptr)
{
}

GameTaskManager::~GameTaskManager()
{
	Reset();
}

void GameTaskManager::AddTask(const GameTask& task)
{
	tasks[task.GetType()] = task;
}

void GameTaskManager::ResolveDependencies()
{
	taskGraph = make_shared<tbb::flow::graph>();
	
	// first construct all the graph nodes
	taskNodes[GameTask::Type::GraphBroadcast] = make_shared<StartNodeType>(*taskGraph);
	for (auto iter = tasks.cbegin(); iter != tasks.cend(); iter++)
	{
		const GameTask::Type taskType = (*iter).first;
		taskNodes[taskType] = make_shared<TaskNodeType>(*taskGraph, tasks[taskType]);;
	}

	// then we can make the graph edges
	for (auto iter = tasks.cbegin(); iter != tasks.cend(); iter++)
	{
		const GameTask::Type taskType = (*iter).first;
		const GameTask& gameTask = (*iter).second;
		if (size_t count = gameTask.GetDependencyCount())
		{
			for(size_t i=0; i<count; i++)
			{
				// edge from dependent on to depending
				TaskNodeType* from = static_cast<TaskNodeType*>(taskNodes[gameTask.GetDependency(i)].get());
				TaskNodeType* to = static_cast<TaskNodeType*>(taskNodes[taskType].get());
				tbb::flow::make_edge(*from, *to);
			}
		}
		else
		{
			// with no dependencies we can kick it off at the start
			StartNodeType* from = static_cast<StartNodeType*>(taskNodes[GameTask::Type::GraphBroadcast].get());
			TaskNodeType* to = static_cast<TaskNodeType*>(taskNodes[taskType].get());
			tbb::flow::make_edge(*from, *to);
		}
	}
}

void GameTaskManager::Run()
{
	StartNodeType* from = static_cast<StartNodeType*>(taskNodes[GameTask::Type::GraphBroadcast].get());
	from->try_put(tbb::flow::continue_msg());
	taskGraph->wait_for_all();
}

void GameTaskManager::Reset()
{
	taskNodes.clear();
	tasks.clear();
}