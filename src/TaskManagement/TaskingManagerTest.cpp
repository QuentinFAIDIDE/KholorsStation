#include "TaskingManager.h"
#include "Task.h"
#include "TaskListener.h"
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <thread>
#include <vector>

/**
 * @brief This class describe a Task that has an identifer.
 */
class TestTask1 : public Task
{
  public:
    TestTask1(std::string id)
    {
        identifier = id;
    }

    std::vector<std::shared_ptr<Task>> getOppositeTasks()
    {
        return std::vector<std::shared_ptr<Task>>();
    }

    std::string marshal()
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "test_task"},
                                {"identifier", identifier},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::string identifier;
};

/**
 * @brief This class describe a Task that has an identifer.
 */
class TestTaskReverse : public Task
{
  public:
    TestTaskReverse(std::string id)
    {
        identifier = id;
    }

    std::vector<std::shared_ptr<Task>> getOppositeTasks()
    {
        std::vector<std::shared_ptr<Task>> reversed;
        reversed.push_back(std::make_shared<TestTaskReverse>("reversed " + identifier));
        return reversed;
    }

    std::string marshal()
    {
        nlohmann::json taskj = {{"object", "task"},
                                {"task", "test_task_reverse"},
                                {"identifier", identifier},
                                {"is_completed", isCompleted()},
                                {"failed", hasFailed()},
                                {"recordable_in_history", recordableInHistory},
                                {"is_part_of_reversion", isPartOfReversion}};
        return taskj.dump();
    }

    std::string identifier;
};

/**
 * @brief This class defines a TaskListener that
 * records whathever it receives and can return
 * history it received in a thread safe manner.
 */
class TestTaskListener : public TaskListener
{
  public:
    TestTaskListener(bool shouldStop) : stopTasksFromDescending(shouldStop)
    {
    }

    bool taskHandler(std::shared_ptr<Task> task)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto reverseTask = std::dynamic_pointer_cast<TestTaskReverse>(task);
        if (reverseTask != nullptr && !reverseTask->isCompleted() && !reverseTask->hasFailed())
        {
            reverseTask->setCompleted(true);
        }
        history.push_back(task);
        spdlog::debug("TEST LISTENER RECORD: " + task->marshal());
        return stopTasksFromDescending;
    }

    std::vector<std::shared_ptr<Task>> getHistory()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::shared_ptr<Task>> historyCopy(history.size());
        std::copy(history.begin(), history.end(), historyCopy.begin());
        return historyCopy;
    }

    bool stopTasksFromDescending;
    std::vector<std::shared_ptr<Task>> history;
    std::mutex mutex;
};

/**
 * @brief This class describe the test suite.
 */
class TestSuite
{
  public:
    TestSuite()
    {
    }
    ~TestSuite()
    {
    }

    // testing that the tasks are submitted to listeners normally and in reasonable time
    void RunTestPostTasks01()
    {
        // create tasking manager
        taskingManager = std::make_unique<TaskingManager>();

        // all tasks should stop at task listener 2
        TestTaskListener taskListener1(false);
        TestTaskListener taskListener2(true);
        TestTaskListener taskListener3(false);

        taskingManager->registerTaskListener(&taskListener1);
        taskingManager->registerTaskListener(&taskListener2);
        taskingManager->registerTaskListener(&taskListener3);

        auto task = std::make_shared<TestTask1>("task1");
        auto task2 = std::make_shared<TestTask1>("task2");
        taskingManager->broadcastTask(task);
        taskingManager->broadcastTask(task2);

        auto historyListener1BeforeStart = taskListener1.getHistory();
        if (historyListener1BeforeStart.size() != 0)
        {
            throw std::runtime_error("history listener 1 received some tasks before taskingManager startup: " +
                                     std::to_string(historyListener1BeforeStart.size()));
        }

        taskingManager->startTaskBroadcast();

        // this method of waiting has a wonky test potential
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto historyListener1 = taskListener1.getHistory();
        auto historyListener2 = taskListener2.getHistory();
        auto historyListener3 = taskListener3.getHistory();

        if (historyListener1.size() != 2)
        {
            throw std::runtime_error("history listener 1 didn't receive the 2 expected tasks, he received " +
                                     std::to_string(historyListener1.size()));
        }
        auto listener1Task1 = std::dynamic_pointer_cast<TestTask1>(historyListener1[0]);
        auto listener1Task2 = std::dynamic_pointer_cast<TestTask1>(historyListener1[1]);
        if (listener1Task1 == nullptr || listener1Task1->identifier != "task1")
        {
            throw std::runtime_error("invalid task1 in first listener");
        }
        if (listener1Task2 == nullptr || listener1Task2->identifier != "task2")
        {
            throw std::runtime_error("invalid task2 in first listener");
        }

        if (historyListener2.size() != 2)
        {
            throw std::runtime_error("history listener 2 didn't receive the 2 expected tasks, he received " +
                                     std::to_string(historyListener2.size()));
        }
        auto listener2Task1 = std::dynamic_pointer_cast<TestTask1>(historyListener2[0]);
        auto listener2Task2 = std::dynamic_pointer_cast<TestTask1>(historyListener2[1]);
        if (listener2Task1 == nullptr || listener2Task1->identifier != "task1")
        {
            throw std::runtime_error("invalid task1 in second listener");
        }
        if (listener2Task2 == nullptr || listener2Task2->identifier != "task2")
        {
            throw std::runtime_error("invalid task2 in second listener");
        }

        if (historyListener3.size() != 0)
        {
            throw std::runtime_error("history listener 3 received tasks but shouldn't have: " +
                                     std::to_string(historyListener3.size()));
        }

        // deleting tasking manager
        taskingManager.reset(nullptr);
    }

    void RunTestCancelRestore01()
    {
        // create tasking manager
        taskingManager = std::make_unique<TaskingManager>();

        // this listener will serve as a witness for task execution
        TestTaskListener taskListener1(false);
        taskingManager->registerTaskListener(&taskListener1);

        auto task = std::make_shared<TestTaskReverse>("task1");
        auto task2 = std::make_shared<TestTaskReverse>("task2");

        taskingManager->startTaskBroadcast();

        // 0 -> TestTaskReverse("task1")
        taskingManager->broadcastTask(task);
        // 1 -> TestTaskReverse("task2")
        taskingManager->broadcastTask(task2);

        auto reverseTask1 = std::make_shared<CancelTask>();
        auto restoreTask1 = std::make_shared<RestoreTask>();
        auto reverseTask2 = std::make_shared<CancelTask>();
        auto reverseTask3 = std::make_shared<CancelTask>();

        // 3 -> CancelTask()
        // 2 -> TestTaskReverse("reversed task2")
        taskingManager->broadcastTask(reverseTask1);
        // 5 -> RestoreTask()
        // 4 -> TestTaskReverse("task2")
        taskingManager->broadcastTask(restoreTask1);
        // 7 -> CancelTask()
        // 6 -> TestTaskReverse("reversed task2")
        taskingManager->broadcastTask(reverseTask2);
        // 9 -> CancelTask()
        // 8 -> TestTaskReverse("reversed task1")
        taskingManager->broadcastTask(reverseTask3);

        auto restoreTask2 = std::make_shared<RestoreTask>();
        auto restoreTask3 = std::make_shared<RestoreTask>();

        // 11 -> RestoreTask()
        // 10 -> TestTaskReverse("task1")
        taskingManager->broadcastTask(restoreTask2);
        // 13 -> RestoreTask()
        // 12 -> TestTaskReverse("task2")
        taskingManager->broadcastTask(restoreTask3);

        auto clearHistoryTask = std::make_shared<ClearHistoryTask>();
        taskingManager->broadcastTask(clearHistoryTask);

        // 14 -> ClearHistoryTask() [Failed]

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto listenerHistory = taskListener1.getHistory();

        auto history0 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[0]);
        auto history1 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[1]);
        auto history2 = std::dynamic_pointer_cast<CancelTask>(listenerHistory[3]);
        auto history3 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[2]);
        auto history4 = std::dynamic_pointer_cast<RestoreTask>(listenerHistory[5]);
        auto history5 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[4]);
        auto history6 = std::dynamic_pointer_cast<CancelTask>(listenerHistory[7]);
        auto history7 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[6]);
        auto history8 = std::dynamic_pointer_cast<CancelTask>(listenerHistory[9]);
        auto history9 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[8]);
        auto history10 = std::dynamic_pointer_cast<RestoreTask>(listenerHistory[11]);
        auto history11 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[10]);
        auto history12 = std::dynamic_pointer_cast<RestoreTask>(listenerHistory[13]);
        auto history13 = std::dynamic_pointer_cast<TestTaskReverse>(listenerHistory[12]);
        auto history14 = std::dynamic_pointer_cast<ClearHistoryTask>(listenerHistory[14]);

        if (history0 == nullptr || !history0->isCompleted() || history0->hasFailed() || history0->identifier != "task1")
        {
            if (history0 == nullptr)
            {
                throw std::runtime_error("unexpected history0: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history0:" + history0->marshal());
            }
        }

        if (history1 == nullptr || !history1->isCompleted() || history1->hasFailed() || history1->identifier != "task2")
        {
            if (history1 == nullptr)
            {
                throw std::runtime_error("unexpected history1: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history1:" + history1->marshal());
            }
        }

        if (history2 == nullptr || !history2->isCompleted() || history2->hasFailed())
        {
            if (history2 == nullptr)
            {
                throw std::runtime_error("unexpected history2: nullptr");
            }
            throw std::runtime_error("unexpected history2:" + history2->marshal());
        }

        if (history3 == nullptr || !history3->isCompleted() || history3->hasFailed() ||
            history3->identifier != "reversed task2")
        {
            if (history3 == nullptr)
            {
                throw std::runtime_error("unexpected history3: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history3:" + history3->marshal());
            }
        }

        if (history4 == nullptr || !history4->isCompleted() || history4->hasFailed())
        {
            if (history4 == nullptr)
            {
                throw std::runtime_error("unexpected history4: nullptr");
            }
            throw std::runtime_error("unexpected history4:" + history4->marshal());
        }

        if (history5 == nullptr || !history5->isCompleted() || history5->hasFailed() || history5->identifier != "task2")
        {
            if (history5 == nullptr)
            {
                throw std::runtime_error("unexpected history5: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history5:" + history5->marshal());
            }
        }

        if (history6 == nullptr || !history6->isCompleted() || history6->hasFailed())
        {
            if (history6 == nullptr)
            {
                throw std::runtime_error("unexpected history6: nullptr");
            }
            throw std::runtime_error("unexpected history6:" + history6->marshal());
        }

        if (history7 == nullptr || !history7->isCompleted() || history7->hasFailed() ||
            history7->identifier != "reversed task2")
        {
            if (history7 == nullptr)
            {
                throw std::runtime_error("unexpected history7: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history7:" + history7->marshal());
            }
        }

        if (history8 == nullptr || !history8->isCompleted() || history8->hasFailed())
        {
            if (history8 == nullptr)
            {
                throw std::runtime_error("unexpected history8: nullptr");
            }
            throw std::runtime_error("unexpected history8:" + history8->marshal());
        }

        if (history9 == nullptr || !history9->isCompleted() || history9->hasFailed() ||
            history9->identifier != "reversed task1")
        {
            if (history9 == nullptr)
            {
                throw std::runtime_error("unexpected history9: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history9:" + history9->marshal());
            }
        }

        if (history10 == nullptr || !history10->isCompleted() || history10->hasFailed())
        {
            if (history10 == nullptr)
            {
                throw std::runtime_error("unexpected history10: nullptr");
            }
            throw std::runtime_error("unexpected history10:" + history10->marshal());
        }

        if (history11 == nullptr || !history11->isCompleted() || history11->hasFailed() ||
            history11->identifier != "task1")
        {
            if (history11 == nullptr)
            {
                throw std::runtime_error("unexpected history11: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history11:" + history11->marshal());
            }
        }

        if (history12 == nullptr || !history12->isCompleted() || history12->hasFailed())
        {
            if (history12 == nullptr)
            {
                throw std::runtime_error("unexpected history12: nullptr");
            }
            throw std::runtime_error("unexpected history12:" + history12->marshal());
        }

        if (history13 == nullptr || !history13->isCompleted() || history13->hasFailed() ||
            history13->identifier != "task2")
        {
            if (history13 == nullptr)
            {
                throw std::runtime_error("unexpected history13: nullptr");
            }
            else
            {
                throw std::runtime_error("unexpected history13:" + history13->marshal());
            }
        }

        if (history14 == nullptr || !history14->isCompleted() || history14->hasFailed())
        {
            if (history14 == nullptr)
            {
                throw std::runtime_error("unexpected history14: nullptr");
            }
            throw std::runtime_error("unexpected history14:" + history14->marshal());
        }

        // deleting tasking manager
        taskingManager.reset(nullptr);
    }

  private:
    std::unique_ptr<TaskingManager> taskingManager;
};

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::trace);
    TestSuite t;
    t.RunTestPostTasks01();
    t.RunTestCancelRestore01();
}