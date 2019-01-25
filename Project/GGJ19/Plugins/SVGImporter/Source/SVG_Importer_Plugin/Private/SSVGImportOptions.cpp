// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "SSVGImportOptions.h"
#include "PropertyEditorModule.h"
#include "Engine/DataTable.h"
#include "Engine/UserDefinedStruct.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "SVGImportFactory"

void SSVGImportOptions::Construct(const FArguments& InArgs)
{
    ImportSettings = InArgs._ImportSettings;
    WidgetWindow = InArgs._WidgetWindow;

    check(ImportSettings);
    
    TSharedPtr<SBox> InspectorBox;
    this->ChildSlot
    [
      SNew(SBorder)
      .BorderImage(FEditorStyle::GetBrush(TEXT("Menu.Background")))
      .Padding(10)
      [
        SNew(SVerticalBox) 
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(3)
        [
          SNew(SBorder)
          .Padding(FMargin(3))
          .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
          [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
              SNew(STextBlock)
              .Font(FEditorStyle::GetFontStyle("CurveEd.LabelFont"))
              .Text(LOCTEXT("Import_CurrentFileTitle", "File path: "))
            ] 
            + SHorizontalBox::Slot()
            .Padding(5, 0, 0, 0)
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
              SNew(STextBlock)
              .Font(FEditorStyle::GetFontStyle("CurveEd.InfoFont"))
              .Text(InArgs._FullPath)
            ]
          ]
        ]

        // Import Settings
        + SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4)
		[
		  SAssignNew(InspectorBox, SBox)
          .MinDesiredWidth(400.0f)
		]

        // Ok/Cancel
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Right)
        .Padding(2, 6, 2, 2)
        [
          SNew(SHorizontalBox)
          + SHorizontalBox::Slot()
          .Padding(2)
          .AutoWidth()
          [
            SNew(SButton)
            .Text(LOCTEXT("Import", "Import"))
            .OnClicked(this, &SSVGImportOptions::OnImport)
          ]
          + SHorizontalBox::Slot()
          .AutoWidth()
          .Padding(2)
          [
            SNew(SButton)
            .Text(LOCTEXT("Cancel", "Cancel"))
            .OnClicked(this, &SSVGImportOptions::OnCancel)
          ]
        ]
      ]
    ];

    // create details view
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    InspectorBox->SetContent(DetailsView->AsShared());
    DetailsView->SetObject(ImportSettings);
}

bool SSVGImportOptions::ShouldImport()
{
    return bImport;
}

/** Called when 'OK' button is pressed */
FReply SSVGImportOptions::OnImport()
{
    bImport = true;
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

/** Called when 'Cancel' button is pressed */
FReply SSVGImportOptions::OnCancel()
{
    bImport = false;
    if (WidgetWindow.IsValid())
    {
        WidgetWindow.Pin()->RequestDestroyWindow();
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
