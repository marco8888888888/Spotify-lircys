// Orbitwerk - Haupt-Spielmodul

#include "OrbitwerkPlayerController.h"

#include "Orbitwerk.h"
#include "OrbitwerkGraphicsSettings.h"
#include "OrbitwerkMainMenuWidget.h"
#include "OrbitwerkSceneHintWidget.h"

#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "OrbitwerkPlayerController"

AOrbitwerkPlayerController::AOrbitwerkPlayerController()
{
	MainMenuWidgetClass = UOrbitwerkMainMenuWidget::StaticClass();
	SceneHintWidgetClass = UOrbitwerkSceneHintWidget::StaticClass();
}

void AOrbitwerkPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (const UOrbitwerkSceneSubsystem* Scenes = UOrbitwerkSceneSubsystem::Get(this))
	{
		CurrentScene = Scenes->GetCurrentScene();
	}

	AddMappingContexts();
	SetupSceneUI();
}

void AOrbitwerkPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearActiveWidget();
	Super::EndPlay(EndPlayReason);
}

void AOrbitwerkPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	EnsureInputObjects();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(BackAction, ETriggerEvent::Started, this, &AOrbitwerkPlayerController::HandleBackAction);
	}
	else
	{
		UE_LOG(LogOrbitwerk, Error,
			TEXT("InputComponent ist kein UEnhancedInputComponent - Config/DefaultInput.ini pruefen (DefaultInputComponentClass)."));
	}
}

void AOrbitwerkPlayerController::EnsureInputObjects()
{
	if (!BackAction)
	{
		// ValueType ist per Default Boolean (digitale Taste) - genau richtig fuer "Zurueck".
		BackAction = NewObject<UInputAction>(this, TEXT("IA_OrbitwerkBack"));
	}

	if (!BaseMappingContext)
	{
		BaseMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_OrbitwerkBase"));
		BaseMappingContext->MapKey(BackAction, EKeys::Escape);
		BaseMappingContext->MapKey(BackAction, EKeys::BackSpace);
	}
}

void AOrbitwerkPlayerController::AddMappingContexts()
{
	EnsureInputObjects();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!InputSubsystem)
	{
		UE_LOG(LogOrbitwerk, Warning, TEXT("Kein EnhancedInputLocalPlayerSubsystem - ist das Plugin EnhancedInput aktiv?"));
		return;
	}

	InputSubsystem->AddMappingContext(BaseMappingContext, 0);
	for (const TObjectPtr<UInputMappingContext>& Context : AdditionalMappingContexts)
	{
		if (Context)
		{
			InputSubsystem->AddMappingContext(Context, 1);
		}
	}
}

void AOrbitwerkPlayerController::SetupSceneUI()
{
	switch (CurrentScene)
	{
	case EOrbitwerkScene::Hangar:
		ShowSceneHint(
			LOCTEXT("HangarTitle", "Hangar"),
			LOCTEXT("HangarHint",
				"Platzhalter-Szene - hier entsteht der Raketen-Editor (T20).\n"
				"[Esc] / [Backspace]: zurück zum Hauptmenü\n"
				"WASD + Maus, Q/E: Debug-Kamera"));
		break;

	case EOrbitwerkScene::Flight:
		ShowSceneHint(
			LOCTEXT("FlightTitle", "Flug"),
			LOCTEXT("FlightHint",
				"Platzhalter-Szene - hier entstehen Flug und Orbit (T21-T23).\n"
				"[Esc] / [Backspace]: zurück zum Hauptmenü\n"
				"WASD + Maus, Q/E: Debug-Kamera"));
		break;

	case EOrbitwerkScene::MainMenu:
	case EOrbitwerkScene::Unknown:
	default:
		// Erststart (nur Standalone/Paket, nicht im Editor): Benchmark -> Preset.
		if (UOrbitwerkGraphicsSettings* GraphicsSettings = UOrbitwerkGraphicsSettings::Get())
		{
			GraphicsSettings->RunFirstStartBenchmarkIfNeeded();
		}
		ShowMainMenu();
		break;
	}
}

void AOrbitwerkPlayerController::ShowMainMenu()
{
	ClearActiveWidget();

	TSubclassOf<UOrbitwerkMainMenuWidget> WidgetClass = MainMenuWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UOrbitwerkMainMenuWidget::StaticClass();
	}

	UOrbitwerkMainMenuWidget* Menu = CreateWidget<UOrbitwerkMainMenuWidget>(this, WidgetClass);
	if (!Menu)
	{
		UE_LOG(LogOrbitwerk, Error, TEXT("Hauptmenue-Widget konnte nicht erzeugt werden."));
		return;
	}

	Menu->OnMenuAction.AddDynamic(this, &AOrbitwerkPlayerController::HandleMenuAction);
	Menu->AddToViewport(0);
	ActiveWidget = Menu;

	FInputModeUIOnly MenuInputMode;
	MenuInputMode.SetWidgetToFocus(Menu->TakeWidget());
	MenuInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(MenuInputMode);
	bShowMouseCursor = true;

	UE_LOG(LogOrbitwerk, Log, TEXT("Hauptmenue angezeigt."));
}

void AOrbitwerkPlayerController::ShowSceneHint(const FText& InTitle, const FText& InHint)
{
	ClearActiveWidget();

	TSubclassOf<UOrbitwerkSceneHintWidget> WidgetClass = SceneHintWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UOrbitwerkSceneHintWidget::StaticClass();
	}

	if (UOrbitwerkSceneHintWidget* Hint = CreateWidget<UOrbitwerkSceneHintWidget>(this, WidgetClass))
	{
		Hint->SetHint(InTitle, InHint);
		Hint->AddToViewport(0);
		ActiveWidget = Hint;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void AOrbitwerkPlayerController::ClearActiveWidget()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void AOrbitwerkPlayerController::ReturnToMainMenu()
{
	if (UOrbitwerkSceneSubsystem* Scenes = UOrbitwerkSceneSubsystem::Get(this))
	{
		Scenes->OpenScene(EOrbitwerkScene::MainMenu);
	}
}

void AOrbitwerkPlayerController::HandleBackAction(const FInputActionValue& Value)
{
	if (CurrentScene == EOrbitwerkScene::Hangar || CurrentScene == EOrbitwerkScene::Flight)
	{
		ReturnToMainMenu();
	}
}

void AOrbitwerkPlayerController::HandleMenuAction(EOrbitwerkMenuAction Action)
{
	UOrbitwerkSceneSubsystem* Scenes = UOrbitwerkSceneSubsystem::Get(this);

	switch (Action)
	{
	case EOrbitwerkMenuAction::OpenHangar:
		if (Scenes)
		{
			Scenes->OpenScene(EOrbitwerkScene::Hangar);
		}
		break;

	case EOrbitwerkMenuAction::StartFlight:
		if (Scenes)
		{
			Scenes->OpenScene(EOrbitwerkScene::Flight);
		}
		break;

	case EOrbitwerkMenuAction::Quit:
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		break;

	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
