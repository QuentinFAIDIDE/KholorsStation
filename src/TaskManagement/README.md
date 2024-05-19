# Task Management

This is a library purpose is to:

- pass information between various classes without direct depedencies in a message-queue way
- Isolate the app Task-performing thread from GUI (avoid freezing GUI on long operations)
- handle undo-redo of tasks by users

The developement pattern is the following:

- Initalize a TaskManager (one for your entire software or plugin) that is responsible for managing the task queue.
- Pass a reference to the TaskManager for you classes to will push tasks.
- Inherit the TaskListener class for your classes to listen to tasks.

For large classes, it may be a good idea to separate the tasks management code into a sidecar class.

## Usage

### Defining tasks

A task is inherited from one of the base tasks classes. It bears the state of the task performing,
and a method to undo (getOppositeTasks) and marshal it for logs. Generally I tend to treat these
as structs and leave members public instead of writing countless accessors.

```c++

// HEADER CODE

/**
 Task to recolor selected sample groups.
 */
class SampleGroupRecolor : public Task
{
  public:
    /**
      Task to recolor selected samples
     */
    SampleGroupRecolor(int colorId);

    /**
      Task to recolor selected samples, but using
      raw color and set of track ids instead (for reversed version)
     */
    SampleGroupRecolor(std::set<int>, std::map<int, juce::Colour>);

    /**
     Dumps the task data to a string as json
     */
    std::string marshal() override;

    /**
      Get the opposite colour change (for undoing)
     */
    std::vector<std::shared_ptr<Task>> getOppositeTasks() override;

    // id of the color to put (from the colorPalette)
    int colorId;
    // the ids of the samples that were changed
    std::set<int> changedSampleIds;
    // the color before the change
    juce::Colour colorPerIds;
    // the color after the change
    juce::Colour newColor;
    // list of colors per id
    std::map<int, juce::Colour> colorsPerId;
    // are we recoloring from raw value or id
    bool colorFromId;
};

// CPP IMPLEMENTATION CODE

// two constructors are defined
SampleGroupRecolor::SampleGroupRecolor(int cid) : colorId(cid), colorFromId(true)
{
}

SampleGroupRecolor::SampleGroupRecolor(std::set<int> ids, std::map<int, juce::Colour> newColors)
    : colorId(-1), changedSampleIds(ids), colorsPerId(newColors), colorFromId(false)
{
}

// this method is convenient for logging
std::string SampleGroupRecolor::marshal()
{
    std::vector<int> ids(changedSampleIds.begin(), changedSampleIds.end());
    json taskj = {{"object", "task"},
                  {"task", "sample_group_recolor"},
                  {"color_id", colorId},
                  {"changed_samples", ids},
                  {"newColor", newColor.toString().toStdString()},
                  {"color_from_id", colorFromId},
                  {"is_completed", isCompleted()},
                  {"failed", hasFailed()},
                  {"recordable_in_history", recordableInHistory},
                  {"is_part_of_reversion", isPartOfReversion}};
    return taskj.dump();
}

// this generate a tasks that does the opposite, for undoing purpose. Note that it returns
// a vector of tasks, and if it returns an empty one, that means that this task cannot be undone
// and will not allow the user to undo it with CTRL+Z.
std::vector<std::shared_ptr<Task>> SampleGroupRecolor::getOppositeTasks()
{
    std::vector<std::shared_ptr<Task>> tasks;
    std::shared_ptr<SampleGroupRecolor> task = std::make_shared<SampleGroupRecolor>(changedSampleIds, colorsPerId);
    task->colorId = -1;
    tasks.push_back(task);
    return tasks;
}

```

### Listening for tasks in TaskListeners

```c++

// inside the TaskListener inherit class implementation

// constructor
TempoGrid::TempoGrid(ActivityManager &am) : activityManager(am)
{
    // this will ensure the task listener will be called on all tasks
    activityManager.registerTaskListener(this);
}

// ....

// taskhandling method (has a dedicated doc section below)
bool ArrangementArea::taskHandler(std::shared_ptr<Task> task)
{
    // ....
}
```

### Task creation and submission to the TaskingManager

GUI classes can instantiate tasks, eventually group them so they are undone together, and push them onto the task queue.
It's important to only push tasks from the JUCE event (GUI) thread, in order to prevent
a return or task push that would happen concurrently to the pushing of a task group.

```c++
// this is the most basic usage of task creation and submittion
bool ArrangementArea::keyPressed(const juce::KeyPress &key)
{
    int viewPosition = viewPositionManager->getViewPosition();
    int viewScale = viewPositionManager->getViewScale();

    // if the space key is pressed, play or pause
    if (key == juce::KeyPress::spaceKey)
    {
        if (mixingBus.isCursorPlaying())
        {
            // Task creation and broadcasting to all other TaskListeners
            // as long as one don't return true in taskHandler.
            auto task = std::make_shared<PlayStateUpdateTask>(false, false);
            activityManager.broadcastTask(task);
        }
    }
}


// this will create grouped tasks that can be undone together
void ArrangementArea::handleLeftButtonDown(const juce::MouseEvent &jme)
{
    if (activityManager.getAppState().getUiState() == UI_STATE_DISPLAY_FREQUENCY_SPLIT_LOCATION)
    {
        // iterate over selected tracks to duplicate everything
        std::set<std::size_t>::iterator it = selectedTracks.begin();

        // this id will be assigned to all tasks so they are undone together
        int groupId = Task::getNewTaskGroupIndex();

        while (it != selectedTracks.end())
        {
            float freq = UnitConverter::verticalPositionToFrequency(lastMouseY, getBounds().getHeight());
            // create a track duplicate from the sample id at *it
            std::shared_ptr<SampleCreateTask> task = std::make_shared<SampleCreateTask>(freq, *it);
            // asigning the id to the task
            task->setTaskGroupIndex(groupId);
            // pushing the task to the task queue
            activityManager.broadcastTask(task);

            it++;
        }
    }
}
```

### Background tasking thread

The background tasking thread owned by the TaskingManager class will execute tasks one by one (so task code should be thread safe).
When a task is executed, each registered TaskListener will have its task processing method called synchronously.

```c++
// theorical code inside the TaskingManager class, executed by the Task thread

// ... wait for the first task

// for each task
while (taskToBroadcast != nullptr)
{
    // iterate over task listeners untill one of them returns true
    for (size_t i = 0; i < taskListeners.size(); i++)
    {
        bool shouldStop = taskListeners[i]->taskHandler(taskToBroadcast);
        if (shouldStop)
        {
            break;
        }
    }

    // if the task is one of the task class that goes in task history, and is completed and not failed, record it.
    if (taskToBroadcast->goesInTaskHistory() && taskToBroadcast->isCompleted() && !taskToBroadcast->hasFailed())
    {
        recordTaskInHistory(taskToBroadcast);
    }
    else
    {
        std::cout << "Unrecorded task: " << taskToBroadcast->marshal() << std::endl;
    }

    // ... pull the next task
}
```

### Task processing methods implementation

The task processing method of the TaskListener will test if the task is one it manages, and execute accordingly, eventually modifying the task state to "completed" or "failed".

```c++
// EXAMPLE 1: basic usage of a taskHandler in a TaskListener that handles 2 different types of tasks
bool ArrangementArea::taskHandler(std::shared_ptr<Task> task)
{
    // this code is executed on the tasking thread owned by the TaskingManager, and is written inside the TaskListener inherited class.

    // this is testing if this tasks is one we listen to, if it's not already completed, and if it's not already failed
    std::shared_ptr<SampleGroupRecolor> colorTask = std::dynamic_pointer_cast<SampleGroupRecolor>(task);
    if (colorTask != nullptr && !colorTask->isCompleted() && !colorTask->hasFailed())
    {
        // this is the task content
        recolorSelection(colorTask);

        // mark the task as completed
        colorTask->setCompleted(true);
        // mark the tasks as non failed (default to false, so unecessary in that situation)
        colorTask->setFailed(false);
        // tells the TaskingManager to stop iterating over task listeners.
        return true;
    }

    // Receives updates about completed tasks for loop position change.
    // Note that order of TaskListeners don't matter as the tasks that are completed and need it
    // are meant to be re-broadcasted as completed by jumping the tasks queue and
    // calling again all TaskListeners with the state "completed".
    auto loopPositionTask = std::dynamic_pointer_cast<LoopMovingTask>(task);
    if (loopPositionTask != nullptr && loopPositionTask->isCompleted())
    {
        loopSectionStartFrame = loopPositionTask->currentLoopBeginFrame;
        loopSectionStopFrame = loopPositionTask->currentLoopEndFrame;
        repaint();
        // this tasks has now no reason to keep being broadcasted to other task listeners.
        return false;
    }

    // false is returned when this task broadcast should keep iterating over task listeners.
    // Very often it's because we don't support that task and it needs to reach a TaskListener
    // that supports it. Note that tasks can theorically reach the end of the TaskListeners list
    // without necessarly having been processed and marked completed, in which case they are not saved
    // in task history and a warning is displayed.
    return false;
}

// EXAMPLE 2: Tasks handler that handle one task, complete it, and before returning, emits a task, called a nested task. Nested Tasks are jumping the tasks queue
// and are not recorded in task history. Note that as you will see in next example, a task received by a task handler
// can be broadcasted as nested, and when it's done iterating over TaskListeners as nested, saved in history by being returned as completed by the original TaskHandler. It's only considered
// nested and unable to reach history while passed by broadcastNestedTaskNow.

bool MixingBus::taskHandler(std::shared_ptr<Task> task)
{
    auto moveTask = std::dynamic_pointer_cast<SampleMovingTask>(task);
    if (moveTask != nullptr && !moveTask->isCompleted() && !moveTask->hasFailed())
    {
        if (samplePlayers[moveTask->id] != nullptr)
        {
            samplePlayers[moveTask->id]->move(samplePlayers[moveTask->id]->getEditingPosition() +
                                              moveTask->dragDistance);
            moveTask->setCompleted(true);
            moveTask->setFailed(false);

            // we need to tell arrangement are to update
            std::shared_ptr<SampleUpdateTask> sut =
                std::make_shared<SampleUpdateTask>(moveTask->id, samplePlayers[moveTask->id]);
            activityManager.broadcastNestedTaskNow(sut);
        }
        else
        {
            moveTask->setCompleted(true);
            moveTask->setFailed(true);
        }
        return true;
    }

    return false
}

// EXAMPLE 3: Sometimes, other TaskListeners should be let known about a task completion.
// This task handler handles one task, but this task has a requestingStateBroadcast boolean
// where it can basically do nothing and just broadcast a value from the TaskListener to other
// TaskListeners. Otherwise it change something and also rebroadcast the task as completed to other TaskListeners.
bool MixingBus::taskHandler(std::shared_ptr<Task> task)
{
    std::shared_ptr<PlayStateUpdateTask> playUpdate = std::dynamic_pointer_cast<PlayStateUpdateTask>(task);
    if (playUpdate != nullptr && !playUpdate->isCompleted())
    {
        // PATTERN 1: The value broadcasting

        // this is a special case of this task, where one class instance requested
        // the state to be broacasted to all TaskListeners again
        if (playUpdate->requestingStateBroadcast)
        {
            // we set the requested value in the task that will be passed with it
            playUpdate->isCurrentlyPlaying = isPlaying;
            // We mark it as completed, if it was a tasks that is has "recordableInHistory" true,
            // it would then go to task history after we return (this one does nothing so it has recordableInHistory as false).
            // Note that if we would return false, other taskListeners may change the task in some way by altering recordableInHistory or completed/failed status
            // and prevent it from going to history, so better return true in that case and for sanity only
            // have a single TaskListener being able to alter completion state of that task :)
            playUpdate->setCompleted(true);
            // this calls all the TaskListeners in the list (eventually untill one returns true)
            // to ensure all TaskListeners who neeed can read the isCurrentlyPlaying value
            activityManager.broadcastNestedTaskNow(playUpdate);
            // stop iterating over TaskListeners for this task
            return true;
        }

        // PATTERN 2: doing something and then value broacasting, or doing nothing and failing

        // if we reach here, it's not a request for update, it's a request for
        // execution of play/pause state change.
        if (playUpdate->shouldPlay)
        {
            // To prevent wasting resources,
            // we won't treat and broadcast state update for update that won't
            // change the state.
            if (isPlaying)
            {
                // this mark this
                playUpdate->setFailed(true);
                // we stop broadcast as no other TaskListener should
                // respond to this cursed useless task.
                return true;
            }
            else
            {
                startPlayback();
                playUpdate->setCompleted(true);
                playUpdate->isCurrentlyPlaying = true;
                activityManager.broadcastNestedTaskNow(playUpdate);
                return true;
            }
        }
    }

    return false
}

```

### Patterns not covered here

- Sometimes but rarely, a tasks needs to go through multiple TaskListeners before being completed, in that
  eventuality, you are required to implement yourself a mechanism where the task is re-broadcasted each time
  with `braodcastNestedTaskNow` until it can be marked as completed based on a state (a count or a set of flags).
  After going through a chain of `braodcastNestedTaskNow`, the tasks will reach the history from the original
  task, and as it's a pointer will bear the "completed" state.

- Some tasks like turning a slider in the GUI, may require a lot of intermediate tasks to update as the mouse moves. Practically speaking
  undoing all those intermediate states makes no sense and is annoying to users, only the entire
  movement should be undone. In that eventuality, the library
  user should implement the task so that its state determines that all intermediate kind of tasks are performed not saved in history,
  are eventually rebroadcasted as completed with `broacastNestedTaskNow` so that GUI continuously redraws, and when the user release the mouse
  button, a final task with a different state that contain the initial and final value is emmited. This final state would not be executed,
  but would be marked completed, therefore being saved in history, and if user press CTRL+Z, it would generate the opposite tasks that would be executed.

### Eventual initialization problems

- The only situation where order of task listeners matter is for initialization. Unfortunately when writing the Kholors
  experimental DAW I encountered this kind of situation where one TaskListeners need the value of some knob, and the task that
  broacast this value was executed when the other TaskListener calss was still not initalized. Effectively this was a mess
  and in the future, I am wondering if I shouldn't allow the library user to "start" the TaskingManager thread only when he initialized
  everything. A basic rule of thumb is to avoid having to manage tasks in constructors.
