// Orbitwerk - OrbitwerkUI
// Kleine Helfer, um UMG-Widgets ohne Blueprint/.uasset direkt in C++ aufzubauen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

namespace OrbitwerkUIStyle
{
	// Eigene Orbitwerk-Farbwelt: tiefes Nachtblau + warmes Triebwerks-Orange.
	inline FLinearColor Background()   { return FLinearColor(0.006f, 0.010f, 0.022f, 1.0f); }
	inline FLinearColor Panel()        { return FLinearColor(0.0f, 0.0f, 0.0f, 0.55f); }
	inline FLinearColor ButtonColor()  { return FLinearColor(0.09f, 0.16f, 0.30f, 1.0f); }
	inline FLinearColor TextPrimary()  { return FLinearColor(0.92f, 0.94f, 0.98f, 1.0f); }
	inline FLinearColor TextMuted()    { return FLinearColor(0.55f, 0.62f, 0.75f, 1.0f); }
	inline FLinearColor Accent()       { return FLinearColor(1.0f, 0.48f, 0.12f, 1.0f); }

	inline FSlateFontInfo Font(float InSize, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", InSize);
	}

	inline UTextBlock* MakeText(UWidgetTree* Tree, const FText& InText, float InSize, bool bBold,
		const FLinearColor& InColor, ETextJustify::Type InJustify = ETextJustify::Center)
	{
		UTextBlock* TextBlock = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TextBlock->SetText(InText);
		TextBlock->SetFont(Font(InSize, bBold));
		TextBlock->SetColorAndOpacity(FSlateColor(InColor));
		TextBlock->SetJustification(InJustify);
		return TextBlock;
	}
}
