#include "ProceduralContentProcessorEdMode.h"
#include "EditorModeManager.h"
#include "ScopedTransaction.h"
#include "Toolkits/ToolkitManager.h"

const FEditorModeID FProceduralContentProcessorEdMode::EdID(TEXT("EM_ProceduralContentProcessor"));

void FProceduralContentProcessorEdMode::Enter()
{
    FEdMode::Enter();

    if (!Toolkit.IsValid())
    {
        Toolkit = MakeShareable(new FProceduralContentProcessorEdModeToolkit);
        Toolkit->Init(Owner->GetToolkitHost());
    }
}

void FProceduralContentProcessorEdMode::Exit()
{
    FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
    Toolkit.Reset();

    FEdMode::Exit();
}

FProceduralContentProcessorEdModeToolkit::FProceduralContentProcessorEdModeToolkit()
{
    Editor = SNew(SProceduralContentProcessorEditorOutliner);
}

FName FProceduralContentProcessorEdModeToolkit::GetToolkitFName() const
{
	return FName("ProceduralContentProcessorEdMode");
}

FText FProceduralContentProcessorEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("ProceduralContentProcessor", "ProceduralContentProcessor", "Procedural Content Processor");
}

class FEdMode* FProceduralContentProcessorEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FProceduralContentProcessorEdMode::EdID);
}

TSharedPtr<class SWidget> FProceduralContentProcessorEdModeToolkit::GetInlineContent() const
{
	return Editor;
}
