// Orbitwerk - OrbitwerkUI

#include "OrbitwerkMainMenuWidget.h"

#include "OrbitwerkGraphicsSettings.h"
#include "OrbitwerkUI.h"
#include "OrbitwerkUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "OrbitwerkMainMenu"

TSharedRef<SWidget> UOrbitwerkMainMenuWidget::RebuildWidget()
{
	// Reine C++-Klasse: WidgetTree existiert (UUserWidget::Initialize), ist aber leer.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UOrbitwerkMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshGraphicsLabel();
}

void UOrbitwerkMainMenuWidget::BuildLayout()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(OrbitwerkUIStyle::Background());
	RootBorder->SetHorizontalAlignment(HAlign_Center);
	RootBorder->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	RootBorder->SetContent(MenuBox);

	// Titel
	UTextBlock* TitleText = OrbitwerkUIStyle::MakeText(WidgetTree, LOCTEXT("Title", "ORBITWERK"), 64.f, true, OrbitwerkUIStyle::TextPrimary());
	if (UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UTextBlock* SubtitleText = OrbitwerkUIStyle::MakeText(WidgetTree,
		LOCTEXT("Subtitle", "Raketenbau im Helion-System"), 20.f, false, OrbitwerkUIStyle::Accent());
	if (UVerticalBoxSlot* SubtitleSlot = MenuBox->AddChildToVerticalBox(SubtitleText))
	{
		SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
		SubtitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
	}

	// Buttons
	UButton* HangarButton = AddMenuButton(MenuBox, LOCTEXT("Hangar", "Hangar"));
	HangarButton->OnClicked.AddDynamic(this, &UOrbitwerkMainMenuWidget::HandleHangarClicked);

	UButton* FlightButton = AddMenuButton(MenuBox, LOCTEXT("Flight", "Flug starten"));
	FlightButton->OnClicked.AddDynamic(this, &UOrbitwerkMainMenuWidget::HandleFlightClicked);

	UTextBlock* GraphicsText = nullptr;
	UButton* GraphicsButton = AddMenuButton(MenuBox, LOCTEXT("GraphicsPlaceholder", "Grafik: ..."), &GraphicsText);
	GraphicsButton->OnClicked.AddDynamic(this, &UOrbitwerkMainMenuWidget::HandleGraphicsClicked);
	GraphicsLabel = GraphicsText;

	UButton* BenchmarkButton = AddMenuButton(MenuBox, LOCTEXT("Benchmark", "Grafik automatisch erkennen"));
	BenchmarkButton->OnClicked.AddDynamic(this, &UOrbitwerkMainMenuWidget::HandleBenchmarkClicked);

	UButton* QuitButton = AddMenuButton(MenuBox, LOCTEXT("Quit", "Beenden"));
	QuitButton->OnClicked.AddDynamic(this, &UOrbitwerkMainMenuWidget::HandleQuitClicked);

	// Statuszeile (Benchmark-Ergebnis, Fehler)
	StatusLabel = OrbitwerkUIStyle::MakeText(WidgetTree, FText::GetEmpty(), 14.f, false, OrbitwerkUIStyle::TextMuted());
	if (UVerticalBoxSlot* StatusSlot = MenuBox->AddChildToVerticalBox(StatusLabel))
	{
		StatusSlot->SetHorizontalAlignment(HAlign_Center);
		StatusSlot->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}

	UTextBlock* VersionText = OrbitwerkUIStyle::MakeText(WidgetTree, LOCTEXT("Version", "Prototyp 0.1 (T00-Scaffold)"), 12.f, false, OrbitwerkUIStyle::TextMuted());
	if (UVerticalBoxSlot* VersionSlot = MenuBox->AddChildToVerticalBox(VersionText))
	{
		VersionSlot->SetHorizontalAlignment(HAlign_Center);
		VersionSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
}

UButton* UOrbitwerkMainMenuWidget::AddMenuButton(UVerticalBox* Box, const FText& InLabel, UTextBlock** OutLabel)
{
	UButton* NewButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	NewButton->SetBackgroundColor(OrbitwerkUIStyle::ButtonColor());

	UTextBlock* LabelText = OrbitwerkUIStyle::MakeText(WidgetTree, InLabel, 24.f, true, OrbitwerkUIStyle::TextPrimary());
	NewButton->SetContent(LabelText);

	if (UVerticalBoxSlot* ButtonSlot = Box->AddChildToVerticalBox(NewButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetPadding(FMargin(0.f, 6.f));
	}

	if (OutLabel)
	{
		*OutLabel = LabelText;
	}
	return NewButton;
}

void UOrbitwerkMainMenuWidget::SetStatus(const FText& InStatus)
{
	if (StatusLabel)
	{
		StatusLabel->SetText(InStatus);
	}
}

void UOrbitwerkMainMenuWidget::RefreshGraphicsLabel()
{
	if (!GraphicsLabel)
	{
		return;
	}

	const UOrbitwerkGraphicsSettings* Settings = UOrbitwerkGraphicsSettings::Get();
	const FText PresetName = Settings
		? UOrbitwerkGraphicsSettings::GetPresetDisplayName(Settings->GetCurrentPreset())
		: LOCTEXT("NoSettings", "nicht verfuegbar");

	GraphicsLabel->SetText(FText::Format(LOCTEXT("GraphicsFmt", "Grafik: {0}"), PresetName));
}

void UOrbitwerkMainMenuWidget::HandleHangarClicked()
{
	OnMenuAction.Broadcast(EOrbitwerkMenuAction::OpenHangar);
}

void UOrbitwerkMainMenuWidget::HandleFlightClicked()
{
	OnMenuAction.Broadcast(EOrbitwerkMenuAction::StartFlight);
}

void UOrbitwerkMainMenuWidget::HandleGraphicsClicked()
{
	UOrbitwerkGraphicsSettings* Settings = UOrbitwerkGraphicsSettings::Get();
	if (!Settings)
	{
		SetStatus(LOCTEXT("SettingsMissing",
			"UOrbitwerkGraphicsSettings nicht aktiv - GameUserSettingsClassName in DefaultEngine.ini pruefen."));
		UE_LOG(LogOrbitwerkUI, Warning, TEXT("UOrbitwerkGraphicsSettings ist nicht die aktive GameUserSettings-Klasse."));
		return;
	}

	const EOrbitwerkGraphicsPreset Next = UOrbitwerkGraphicsSettings::GetNextPreset(Settings->GetCurrentPreset());
	Settings->ApplyPreset(Next);
	RefreshGraphicsLabel();
	SetStatus(FText::GetEmpty());
}

void UOrbitwerkMainMenuWidget::HandleBenchmarkClicked()
{
	UOrbitwerkGraphicsSettings* Settings = UOrbitwerkGraphicsSettings::Get();
	if (!Settings)
	{
		HandleGraphicsClicked(); // zeigt die Fehlermeldung
		return;
	}

	const EOrbitwerkGraphicsPreset Suggested = Settings->RunBenchmarkAndSuggestPreset();
	Settings->ApplyPreset(Suggested);
	RefreshGraphicsLabel();

	SetStatus(FText::Format(
		LOCTEXT("BenchmarkFmt", "Benchmark: CPU {0} / GPU {1}  ->  Preset {2}"),
		FText::AsNumber(FMath::RoundToInt(Settings->GetLastCPUBenchmarkResult())),
		FText::AsNumber(FMath::RoundToInt(Settings->GetLastGPUBenchmarkResult())),
		UOrbitwerkGraphicsSettings::GetPresetDisplayName(Suggested)));
}

void UOrbitwerkMainMenuWidget::HandleQuitClicked()
{
	OnMenuAction.Broadcast(EOrbitwerkMenuAction::Quit);
}

#undef LOCTEXT_NAMESPACE
