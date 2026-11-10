// Orbitwerk - OrbitwerkUI

#include "OrbitwerkSceneHintWidget.h"

#include "OrbitwerkUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UOrbitwerkSceneHintWidget::SetHint(const FText& InTitle, const FText& InHint)
{
	PendingTitle = InTitle;
	PendingHint = InHint;

	if (TitleTextBlock)
	{
		TitleTextBlock->SetText(PendingTitle);
	}
	if (HintTextBlock)
	{
		HintTextBlock->SetText(PendingHint);
	}
}

TSharedRef<SWidget> UOrbitwerkSceneHintWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UOrbitwerkSceneHintWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Reines Overlay: keine Maus-/Fokus-Interaktion.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UOrbitwerkSceneHintWidget::BuildLayout()
{
	// Unsichtbarer Vollbild-Rahmen, der das Panel oben links platziert.
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	RootBorder->SetHorizontalAlignment(HAlign_Left);
	RootBorder->SetVerticalAlignment(VAlign_Top);
	RootBorder->SetPadding(FMargin(24.f));
	WidgetTree->RootWidget = RootBorder;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetBrushColor(OrbitwerkUIStyle::Panel());
	PanelBorder->SetPadding(FMargin(16.f, 12.f));
	RootBorder->SetContent(PanelBorder);

	UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TextBox"));
	PanelBorder->SetContent(TextBox);

	TitleTextBlock = OrbitwerkUIStyle::MakeText(WidgetTree, PendingTitle, 32.f, true,
		OrbitwerkUIStyle::Accent(), ETextJustify::Left);
	TextBox->AddChildToVerticalBox(TitleTextBlock);

	HintTextBlock = OrbitwerkUIStyle::MakeText(WidgetTree, PendingHint, 16.f, false,
		OrbitwerkUIStyle::TextPrimary(), ETextJustify::Left);
	if (UVerticalBoxSlot* HintSlot = TextBox->AddChildToVerticalBox(HintTextBlock))
	{
		HintSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
}
