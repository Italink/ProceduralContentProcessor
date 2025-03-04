#pragma once
#include "EditorModes.h"
#include "EdMode.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "SProceduralContentProcessorEditorOutliner.h"

class FProceduralContentProcessorEdMode : public FEdMode
{
public:
    const static FEditorModeID EdID;
    void Enter() override;
    void Exit() override;
};

class FProceduralContentProcessorEdModeToolkit : public FModeToolkit
{
public:
    FProceduralContentProcessorEdModeToolkit();
    /** IToolkit interface */
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual class FEdMode* GetEditorMode() const override;
    virtual TSharedPtr<class SWidget> GetInlineContent() const override;
private:
    TSharedPtr<SProceduralContentProcessorEditorOutliner> Editor;
};