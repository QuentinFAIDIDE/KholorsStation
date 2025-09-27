#include "VolumeOverTimeGraph.h"
#include "StationApp/Audio/TrackInfoStore.h"
#include "TaskManagement/TaskingManager.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_opengl/juce_opengl.h"
#include <optional>

VolumeOverTimeGraph::VolumeOverTimeGraph(TrackInfoStore &tis)
    : trackInfoStore(tis), playCursorPosition(0), timeSignatureGrid(false), topBeatGrid(true), ignoreNewData(true),
      viewPosition(0), viewScale(150), bpm(120), needToResetTiles(false)
{
    setInterceptsMouseClicks(false, false);

    timeSignature = 4;
    lastAppliedTimeSignature = 4;
    backgroundColor = KHOLORS_COLOR_BACKGROUND;
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    mouseOnComponent = false;
    lastMouseX = 0;
    lastMouseY = 0;
    renderOpenGlIter = 0;
    glThreadUniformsNonce = 0;
    lastUsedGlThreadUnifNonce = 0;
}

VolumeOverTimeGraph::~VolumeOverTimeGraph()
{
}

void VolumeOverTimeGraph::paint(juce::Graphics &g)
{
    // TODO: Implement paint method
}

void VolumeOverTimeGraph::paintOverChildren(juce::Graphics &g)
{
    // TODO: Implement paint over children
}

void VolumeOverTimeGraph::resized()
{
    // TODO: Handle component resize
}

void VolumeOverTimeGraph::updateViewPosition(uint32_t samplePosition)
{
    // TODO: Update view position
}

void VolumeOverTimeGraph::updateViewScale(uint32_t samplesPerPixel)
{
    // TODO: Update view scale
}

void VolumeOverTimeGraph::updateBpm(float newBpm, TaskingManager *tm)
{
    // TODO: Update BPM
}

void VolumeOverTimeGraph::timeSignatureNumeratorUpdate(int timeSignatureNumerator)
{
    // TODO: Update time signature
}

void VolumeOverTimeGraph::clearDisplayedSections()
{
    // TODO: Clear displayed sections
}

void VolumeOverTimeGraph::newOpenGLContextCreated()
{
    // TODO: Initialize OpenGL context
}

void VolumeOverTimeGraph::renderOpenGL()
{
    // TODO: Render OpenGL content
}

void VolumeOverTimeGraph::openGLContextClosing()
{
    // TODO: Cleanup OpenGL context
}

void VolumeOverTimeGraph::setMouseCursor(bool onComponent, int x, int y)
{
    // TODO: Handle mouse cursor
}

void VolumeOverTimeGraph::setSelectedTrack(std::optional<uint64_t> selectedTrack, TaskingManager *tm)
{
    // TODO: Set selected track
}