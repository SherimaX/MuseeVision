#include "UI/SMuseePlacard.h"

#include "Catalog/MuseeCatalog.h"
#include "UI/MuseeStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SMuseePlacard::Construct(const FArguments& InArgs)
{
	// Ink: dark bronze, a little softer for the secondary lines.
	const FLinearColor Ink = MuseeStyle::Alpha(MuseeStyle::Bronze, 0.94f);
	const FLinearColor Secondary = MuseeStyle::Alpha(MuseeStyle::Bronze, 0.70f);
	const FLinearColor Quiet = MuseeStyle::Alpha(MuseeStyle::Bronze, 0.56f);

	ArtistLine = MakeLine(MuseeStyle::Font("Medium", MuseeStyle::ArtistSize, 200), MuseeStyle::Alpha(MuseeStyle::Bronze, 0.86f));
	ArtistLine->SetTransformPolicy(ETextTransformPolicy::ToUpper);
	TitleLine = MakeLine(MuseeStyle::Font("Italic", MuseeStyle::TitleSize), Ink, 1.05f);
	OriginalLine = MakeLine(MuseeStyle::Font("Italic", MuseeStyle::OriginalSize), Quiet);
	YearLine = MakeLine(MuseeStyle::Font("Light", MuseeStyle::YearSize, 20), Ink);
	MediumLine = MakeLine(MuseeStyle::Font("Light", MuseeStyle::DetailSize), Secondary);
	CollectionLine = MakeLine(MuseeStyle::Font("Light", MuseeStyle::DetailSize), Secondary);
	NotesLine = MakeLine(MuseeStyle::Font("Light", MuseeStyle::NotesSize), MuseeStyle::Alpha(MuseeStyle::Bronze, 0.82f), 1.3f);
	NotesRule = MuseeStyle::MakeRule(MuseeStyle::Alpha(MuseeStyle::Bronze, 0.12f), 0.f, 1.f);

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(MuseeStyle::LabelMeasure + 64.f)
		[
			SNew(SMuseePanel)
			.Fill(MuseeStyle::Alpha(MuseeStyle::Paper, 0.97f))
			.Outline(MuseeStyle::Alpha(MuseeStyle::Bronze, 0.08f))
			.OutlineWidth(1.f)
			.Radius(2.f)
			.Shadow(0.3f)
			.Padding(FMargin(32.f, 28.f, 32.f, 30.f))
			[
				SNew(SVerticalBox)
				// The gilt rule: the logo's slit of light.
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
				[
					MuseeStyle::MakeRule(MuseeStyle::Gilt, 36.f, 2.f)
				]
				// Who, what, when.
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 18.f, 0.f, 0.f)) [ ArtistLine.ToSharedRef() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 8.f, 0.f, 0.f)) [ TitleLine.ToSharedRef() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 3.f, 0.f, 0.f)) [ OriginalLine.ToSharedRef() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f)) [ YearLine.ToSharedRef() ]
				// The tombstone: medium, then where the work lives.
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 0.f)) [ MediumLine.ToSharedRef() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 2.f, 0.f, 0.f)) [ CollectionLine.ToSharedRef() ]
				// The note, under a hairline.
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 18.f, 0.f, 0.f)) [ NotesRule.ToSharedRef() ]
				+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 14.f, 0.f, 0.f)) [ NotesLine.ToSharedRef() ]
			]
		]
	];
}

void SMuseePlacard::SetWork(const FMuseeArtwork& Work)
{
	ShowLine(*ArtistLine, Work.Artist);
	ShowLine(*TitleLine, Work.Title);
	// The original title only when it says something the title does not.
	ShowLine(*OriginalLine, Work.OriginalTitle.Equals(Work.Title, ESearchCase::IgnoreCase) ? FString() : Work.OriginalTitle);
	ShowLine(*YearLine, Work.Year);
	ShowLine(*MediumLine, Work.Medium);
	// "Musée d'Orsay, Paris"; the city once only ("Museum of Fine Arts, Boston").
	const bool bCityNeeded = !Work.City.IsEmpty() && !Work.Collection.Contains(Work.City);
	ShowLine(*CollectionLine, Work.Collection.IsEmpty() ? Work.City
		: (bCityNeeded ? Work.Collection + TEXT(", ") + Work.City : Work.Collection));
	ShowLine(*NotesLine, Work.Notes);
	NotesRule->SetVisibility(Work.Notes.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
}

TSharedRef<STextBlock> SMuseePlacard::MakeLine(const FSlateFontInfo& InFont, const FLinearColor& InColour, float LineHeight)
{
	return SNew(STextBlock)
		.Font(InFont)
		.ColorAndOpacity(InColour)
		.WrapTextAt(MuseeStyle::LabelMeasure)
		.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
		.LineHeightPercentage(LineHeight)
		.ApplyLineHeightToBottomLine(false)
		.Visibility(EVisibility::Collapsed);
}

void SMuseePlacard::ShowLine(STextBlock& Line, const FString& Text)
{
	Line.SetText(FText::FromString(Text));
	Line.SetVisibility(Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
}
