#include "Terminals/AresTerminalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "AresTerminal"

namespace
{
// Flight-controller palette: near-black plate, cool white type, amber accent.
// Tone per brief §1 — grounded and procedural, not arcade.
const FLinearColor PlateColor(0.02f, 0.03f, 0.04f, 0.94f);
const FLinearColor TitleColor(0.86f, 0.92f, 1.00f, 1.0f);
const FLinearColor SubtitleColor(0.55f, 0.66f, 0.78f, 1.0f);
const FLinearColor BodyColor(0.72f, 0.55f, 0.20f, 1.0f);
} // namespace

FText UAresTerminalWidget::GetTitleFor(ETerminalKind Kind)
{
	switch (Kind)
	{
	case ETerminalKind::VehicleAssembly:
		return LOCTEXT("VabTitle", "VEHICLE CONFIGURATION & MANIFEST");
	case ETerminalKind::MissionControl:
		return LOCTEXT("MccTitle", "FLIGHT DIRECTOR");
	case ETerminalKind::Research:
		return LOCTEXT("RndTitle", "RESEARCH & DEVELOPMENT");
	case ETerminalKind::Administration:
		return LOCTEXT("HqTitle", "PROGRAM ADMINISTRATION");
	default:
		return LOCTEXT("UnknownTitle", "TERMINAL");
	}
}

FText UAresTerminalWidget::GetSubtitleFor(ETerminalKind Kind)
{
	switch (Kind)
	{
	case ETerminalKind::VehicleAssembly:
		return LOCTEXT("VabSub",
			"Payload mass and volume budget  ·  cargo selection  ·  tanker / crew / cargo / robotic variant  ·  ship assignment");
	case ETerminalKind::MissionControl:
		return LOCTEXT("MccSub",
			"Mission timeline  ·  telemetry  ·  launch commit  ·  timewarp");
	case ETerminalKind::Research:
		return LOCTEXT("RndSub",
			"Tech tree  ·  research point allocation  ·  minimum durations  ·  maturity");
	case ETerminalKind::Administration:
		return LOCTEXT("HqSub",
			"Budget  ·  appropriations  ·  contracts and contractors  ·  public support  ·  press");
	default:
		return FText::GetEmpty();
	}
}

void UAresTerminalWidget::SetTerminalKind(ETerminalKind InKind)
{
	Kind = InKind;
	ApplyKindToWidgets();
}

void UAresTerminalWidget::ApplyKindToWidgets()
{
	if (TitleText)
	{
		TitleText->SetText(GetTitleFor(Kind));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(GetSubtitleFor(Kind));
	}
}

TSharedRef<SWidget> UAresTerminalWidget::RebuildWidget()
{
	if (!RootBorder && WidgetTree)
	{
		RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
		WidgetTree->RootWidget = RootBorder;

		RootBorder->SetBrushColor(PlateColor);
		RootBorder->SetPadding(FMargin(120.0f, 90.0f));
		RootBorder->SetHorizontalAlignment(HAlign_Fill);
		RootBorder->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Column =
			WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		RootBorder->SetContent(Column);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 40));
		TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(TitleText))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
		SubtitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
		SubtitleText->SetColorAndOpacity(FSlateColor(SubtitleColor));
		SubtitleText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(SubtitleText))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 48.0f));
		}

		BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
		BodyText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 22));
		BodyText->SetColorAndOpacity(FSlateColor(BodyColor));
		BodyText->SetText(LOCTEXT("NoData",
			"NO DATA\n\nThis console is wired to the program simulation in M2.\n"
			"Nothing behind this screen is running yet."));
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(BodyText))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 48.0f));
		}

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseLabel =
			WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
		CloseLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
		CloseLabel->SetText(LOCTEXT("Close", "  CLOSE  [E]  "));
		CloseButton->SetContent(CloseLabel);
		CloseButton->OnClicked.AddDynamic(this, &UAresTerminalWidget::HandleCloseClicked);

		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(CloseButton))
		{
			Slot->SetHorizontalAlignment(HAlign_Left);
		}

		ApplyKindToWidgets();
	}

	return Super::RebuildWidget();
}

void UAresTerminalWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
