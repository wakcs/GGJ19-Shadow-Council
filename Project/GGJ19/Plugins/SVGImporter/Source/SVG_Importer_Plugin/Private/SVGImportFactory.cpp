// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "SVGImportFactory.h"
#include "Engine/UserDefinedStruct.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "SSVGImportOptions.h"
#define NANOSVG_ALL_COLOR_KEYWORDS  // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION      // Expands implementation
#define NANOSVGRAST_IMPLEMENTATION
#define MSDFGEN_USE_CPP11
#define REQUIRE(cond) { if (!(cond)) return false; }
#define ENDPOINT_SNAP_RANGE_PROPORTION (1/16384.)
#include "nanosvg.h"
#include "nanosvgrast.h"
#include "msdfgen.h"
#include "../Public/SVGPluginRuntimeSettings.h"

using namespace std;
using namespace msdfgen;

DEFINE_LOG_CATEGORY(LogSVGImportFactory);

#define LOCTEXT_NAMESPACE "SVGImportFactory"

//////////////////////////////////////////////////////////////////////////

USVGImportFactory::USVGImportFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bCreateNew = false;
    bEditAfterNew = true;
    SupportedClass = UTexture2D::StaticClass();
    bEditorImport = true;
    bText = true;
    ImportPriority++;
    Formats.Add(TEXT("svg;Scalable vector graphics"));
}

FText USVGImportFactory::GetDisplayName() const
{
    return LOCTEXT("SVGImportFactoryDescription", "Scalable Vector Graphics");
}

bool USVGImportFactory::DoesSupportClass(UClass * Class)
{
    return Class == UTexture2D::StaticClass();
}

bool USVGImportFactory::FactoryCanImport(const FString& Filename)
{
    FString Extension = FPaths::GetExtension(Filename);
    return Extension.Equals(TEXT("svg"), ESearchCase::IgnoreCase);
}

UObject* USVGImportFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
    FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, Type);

    // See if texture already exists
    UTexture2D* ExistingTexture = FindObject<UTexture2D>(InParent, *InName.ToString());
    FSVGAssetSettings settings;
    auto uiSettings = USVGImportSettings::Get();

    if (ExistingTexture && ExistingTexture->PlatformData)
    {
        USVGAssetImportData* existingData = Cast<USVGAssetImportData>(ExistingTexture->AssetImportData);
        if (existingData) {
            settings = existingData->ImportSettings;

            if (!settings.UseSvgResolution) {
                settings.ImportResolution = dimensionToEnum(ExistingTexture->PlatformData->SizeX);
            }
            uiSettings->ImportResolution = settings.ImportResolution;
            uiSettings->UseSvgResolution = settings.UseSvgResolution;
            uiSettings->ImportType = settings.ImportType;
            uiSettings->angleThresholdRadians = settings.AngleThreshold;
            uiSettings->edgeThreshold = settings.EdgeThreshold;
            uiSettings->pixelRange = settings.Range;
            uiSettings->TranslationSDF = settings.TranslationSDF;
            uiSettings->ConvertAllElementsToPath = settings.ConvertAllElementsToPath;
        }
    }

    bool bDoImport = true;
    bool silentImport = GetDefault<USVGPluginRuntimeSettings>()->bSilentReimport;

    // ask for necessary info
    if (!silentImport || !ExistingTexture)
    {
        TSharedPtr<SWindow> ParentWindow;
        TSharedPtr<SSVGImportOptions> ImportOptionsWindow;

        TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(LOCTEXT("SVGOptionsWindowTitle", "SVG Import Options"))
            .SizingRule(ESizingRule::Autosized);

        Window->SetContent
        (            
            SAssignNew(ImportOptionsWindow, SSVGImportOptions)
            .WidgetWindow(Window)
            .FullPath(FText::FromString(CurrentFilename))
            .ImportSettings(uiSettings)
        );

        FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

        settings.ImportResolution = uiSettings->ImportResolution;
        settings.UseSvgResolution = uiSettings->UseSvgResolution;
        settings.ImportType = uiSettings->ImportType;
        settings.AngleThreshold = uiSettings->angleThresholdRadians;
        settings.EdgeThreshold = uiSettings->edgeThreshold;
        settings.Range = uiSettings->pixelRange;
        settings.TranslationSDF = uiSettings->TranslationSDF;
        settings.ConvertAllElementsToPath = uiSettings->ConvertAllElementsToPath;
        bDoImport = ImportOptionsWindow->ShouldImport();
    }

    UObject* NewAsset = NULL;
    if (bDoImport)
    {
        // Convert buffer to an FString (will this be slow with big svg?)
        FString RawData;
        int32 NumChars = (BufferEnd - Buffer);
        TArray<TCHAR>& StringChars = RawData.GetCharArray();
        StringChars.AddUninitialized(NumChars + 1);
        FMemory::Memcpy(StringChars.GetData(), Buffer, NumChars * sizeof(TCHAR));
        StringChars.Last() = 0;
        
        // Parse SVG and get pixel data
        vector<uint8> PixelData;
        auto importResult = ImportSVG(PixelData, RawData, settings);

        // Create/reset texture
        UClass* TextureClass = UTexture2D::StaticClass();
        if (ExistingTexture)
        {
            TextureClass = ExistingTexture->GetClass();
            ExistingTexture->ClearAllCachedCookedPlatformData();
        }

        if (!ExistingTexture) {
            ExistingTexture = NewObject<UTexture2D>(InParent, InClass, InName, Flags);
            USVGAssetImportData* importData = NewObject<USVGAssetImportData>(ExistingTexture, USVGAssetImportData::StaticClass());
            importData->ImportSettings = settings;
            importData->Update(CurrentFilename);
            ExistingTexture->AssetImportData = importData;

            if (settings.ImportType != ESVGImportType::ESVG_ColorTexture) {
                ExistingTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
                ExistingTexture->SRGB = 0;
                ExistingTexture->AddressX = TextureAddress::TA_Mirror;
                ExistingTexture->AddressY = TextureAddress::TA_Mirror;
                ExistingTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
            }
        }
        else {
            USVGAssetImportData* importData = Cast<USVGAssetImportData>(ExistingTexture->AssetImportData);
            if (importData) {
                importData->Update(CurrentFilename);
                importData->ImportSettings = settings;
            }
        }

        if (settings.UseSvgResolution) {
            int32 DimensionX = importResult.svgWidth;
            int32 DimensionY = importResult.svgHeight;
            ExistingTexture->Source.Init(DimensionX, DimensionY, 1, 1, enumToTextureFormat(settings.ImportType), PixelData.data());
        }
        else {
            int32 Dimension = enumToDimension(settings.ImportResolution);
            ExistingTexture->Source.Init(Dimension, Dimension, 1, 1, enumToTextureFormat(settings.ImportType), PixelData.data());
        }
        
        
        ExistingTexture->PostEditChange();

        // Print out
        auto& Problems = importResult.Problems;
        UE_LOG(LogSVGImportFactory, Log, TEXT("Imported SVG '%s' - %d Problems"), *InName.ToString(), Problems.Num());
        NewAsset = ExistingTexture;

        if (Problems.Num() > 0)
        {
            FString AllProblems;

            for (int32 ProbIdx = 0; ProbIdx < Problems.Num(); ProbIdx++)
            {
                // Output problems to log
                UE_LOG(LogSVGImportFactory, Error, TEXT("%d: %s"), ProbIdx, *Problems[ProbIdx]);
                AllProblems += Problems[ProbIdx];
                AllProblems += TEXT("\n");
            }
            // Pop up any problems for user
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(AllProblems));
        }
    }

    FEditorDelegates::OnAssetPostImport.Broadcast(this, NewAsset);
    return NewAsset;
}

bool USVGImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UTexture2D* Tex = Cast<UTexture2D>(Obj);
    if (Tex)
    {
        auto importFile = Tex->AssetImportData->GetFirstFilename();
        if (!FactoryCanImport(importFile)) {
            return false;
        }
        OutFilenames.Add(importFile);
        return true;
    }
    return false;
}

void USVGImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UTexture2D* Tex = Cast<UTexture2D>(Obj);
    if (Tex && ensure(NewReimportPaths.Num() == 1))
    {
        Tex->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
    }
}

EReimportResult::Type USVGImportFactory::Reimport(UObject * Obj)
{
    if (UTexture2D* Tex = Cast<UTexture2D>(Obj))
    {
        Reimport(Tex, Tex->AssetImportData->GetFirstFilename());
        return EReimportResult::Succeeded;
    }
    return EReimportResult::Failed;
}

int32 USVGImportFactory::GetPriority() const
{
    return ImportPriority;
}

static bool readNodeType(char &output, const char *&pathDef) {
    int shift;
    char nodeType;
    if (sscanf(pathDef, " %c%n", &nodeType, &shift) == 1 && nodeType != '+' && nodeType != '-' && nodeType != '.' && nodeType != ',' && (nodeType < '0' || nodeType > '9')) {
        pathDef += shift;
        output = nodeType;
        return true;
    }
    return false;
}

static bool readCoord(Point2 &output, const char *&pathDef) {
    int shift;
    double x, y;
    if (sscanf(pathDef, "%lf%lf%n", &x, &y, &shift) == 2) {
        output.x = x;
        output.y = y;
        pathDef += shift;
        return true;
    }
    if (sscanf(pathDef, "%lf,%lf%n", &x, &y, &shift) == 2) {
        output.x = x;
        output.y = y;
        pathDef += shift;
        return true;
    }
    return false;
}

static bool readDouble(double &output, const char *&pathDef) {
    int shift;
    double v;
    if (sscanf(pathDef, "%lf%n", &v, &shift) == 1) {
        pathDef += shift;
        output = v;
        return true;
    }
    return false;
}

static bool readBool(bool &output, const char *&pathDef) {
    int shift;
    int v;
    if (sscanf(pathDef, "%d%n", &v, &shift) == 1) {
        pathDef += shift;
        output = v != 0;
        return true;
    }
    return false;
}

static Point2 xformPoint(Point2 p, XForm xform) {
    float newX = 0, newY = 0;
    xform.point(p.x, p.y, newX, newY);
    return Point2(newX, newY);
}

static double toDegrees(double rad) {
    return rad * 180.0 / PI;
}

static double toRad(double degrees) {
    return degrees * PI / 180.0;
}

static double arcAngle(Vector2 u, Vector2 v) {
    return nonZeroSign(crossProduct(u, v))*acos(clamp(dotProduct(u, v) / (u.length()*v.length()), -1., +1.));
}

static Vector2 rotateVector(Vector2 v, Vector2 direction) {
    return Vector2(direction.x*v.x - direction.y*v.y, direction.y*v.x + direction.x*v.y);
}

static void addArcApproximate(Contour &contour, Point2 startPoint, Point2 endPoint, Vector2 radius, double rotation, bool largeArc, bool sweep, std::function<Point2(Point2&)> tr) {
    if (endPoint == startPoint)
        return;
    if (radius.x == 0 || radius.y == 0)
        return contour.addEdge(new LinearSegment(tr(startPoint), tr(endPoint)));

    radius.x = fabs(radius.x);
    radius.y = fabs(radius.y);
    Vector2 axis(cos(rotation), sin(rotation));

    Vector2 rm = rotateVector(.5*(startPoint - endPoint), Vector2(axis.x, -axis.y));
    Vector2 rm2 = rm*rm;
    Vector2 radius2 = radius*radius;
    double radiusGap = rm2.x / radius2.x + rm2.y / radius2.y;
    if (radiusGap > 1) {
        radius *= sqrt(radiusGap);
        radius2 = radius*radius;
    }
    double dq = (radius2.x*rm2.y + radius2.y*rm2.x);
    double pq = radius2.x*radius2.y / dq - 1;
    double q = (largeArc == sweep ? -1 : +1)*sqrt(max(pq, 0.));
    Vector2 rc(q*radius.x*rm.y / radius.y, -q*radius.y*rm.x / radius.x);
    Point2 center = .5*(startPoint + endPoint) + rotateVector(rc, axis);

    double angleStart = arcAngle(Vector2(1, 0), (rm - rc) / radius);
    double angleExtent = arcAngle((rm - rc) / radius, (-rm - rc) / radius);
    if (!sweep && angleExtent > 0)
        angleExtent -= 2 * PI;
    else if (sweep && angleExtent < 0)
        angleExtent += 2 * PI;

    int segments = (int)ceil(2 / PI*fabs(angleExtent));
    double angleIncrement = angleExtent / segments;
    double cl = 4 / 3.*sin(.5*angleIncrement) / (1 + cos(.5*angleIncrement));

    Point2 prevNode = startPoint;
    double angle = angleStart;
    for (int i = 0; i < segments; ++i) {
        Point2 controlPoint[2];
        Vector2 d(cos(angle), sin(angle));
        controlPoint[0] = center + rotateVector(Vector2(d.x - cl*d.y, d.y + cl*d.x)*radius, axis);
        angle += angleIncrement;
        d.set(cos(angle), sin(angle));
        controlPoint[1] = center + rotateVector(Vector2(d.x + cl*d.y, d.y - cl*d.x)*radius, axis);
        Point2 node = i == segments - 1 ? endPoint : center + rotateVector(d*radius, axis);
        contour.addEdge(new CubicSegment(tr(prevNode), tr(controlPoint[0]), tr(controlPoint[1]), tr(node)));
        prevNode = node;
    }
}

static bool buildFromPath(Shape &shape, const char *pathDef, XForm xform, NSVGimage* image) {
    char nodeType = '\0';
    char prevNodeType = '\0';
    Point2 prevNode(0, 0);
    bool nodeTypePreread = false;
    double size = sqrt(pow(image->width, 2) + pow(image->height, 2));
    auto tr = [&xform, image](Point2& p) {
        auto newPoint = xformPoint(p, xform);
        return Point2(newPoint.x * image->viewScaleX, newPoint.y * image->viewScaleY);
    };
    while (nodeTypePreread || readNodeType(nodeType, pathDef)) {
        nodeTypePreread = false;
        Contour &contour = shape.addContour();
        bool contourStart = true;

        Point2 startPoint;
        Point2 controlPoint[2];
        Point2 node;

        while (*pathDef) {
            switch (nodeType) {
            case 'M': case 'm':
                if (!contourStart) {
                    nodeTypePreread = true;
                    goto NEXT_CONTOUR;
                }
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 'm')
                    node += prevNode;
                startPoint = node;
                --nodeType; // to 'L' or 'l'
                break;
            case 'Z': case 'z':
                REQUIRE(!contourStart);
                goto NEXT_CONTOUR;
            case 'L': case 'l':
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 'l')
                    node += prevNode;
                contour.addEdge(new LinearSegment(tr(prevNode), tr(node)));
                break;
            case 'H': case 'h':
                REQUIRE(readDouble(node.x, pathDef));
                if (nodeType == 'h')
                    node.x += prevNode.x;
                contour.addEdge(new LinearSegment(tr(prevNode), tr(node)));
                break;
            case 'V': case 'v':
                REQUIRE(readDouble(node.y, pathDef));
                if (nodeType == 'v')
                    node.y += prevNode.y;
                contour.addEdge(new LinearSegment(tr(prevNode), tr(node)));
                break;
            case 'Q': case 'q':
                REQUIRE(readCoord(controlPoint[0], pathDef));
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 'q') {
                    controlPoint[0] += prevNode;
                    node += prevNode;
                }
                contour.addEdge(new QuadraticSegment(tr(prevNode), tr(controlPoint[0]), tr(node)));
                break;
            case 'T': case 't':
                if (prevNodeType == 'Q' || prevNodeType == 'q' || prevNodeType == 'T' || prevNodeType == 't')
                    controlPoint[0] = node + node - controlPoint[0];
                else
                    controlPoint[0] = node;
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 't')
                    node += prevNode;
                contour.addEdge(new QuadraticSegment(tr(prevNode), tr(controlPoint[0]), tr(node)));
                break;
            case 'C': case 'c':
                REQUIRE(readCoord(controlPoint[0], pathDef));
                REQUIRE(readCoord(controlPoint[1], pathDef));
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 'c') {
                    controlPoint[0] += prevNode;
                    controlPoint[1] += prevNode;
                    node += prevNode;
                }
                contour.addEdge(new CubicSegment(tr(prevNode), tr(controlPoint[0]), tr(controlPoint[1]), tr(node)));
                break;
            case 'S': case 's':
                if (prevNodeType == 'C' || prevNodeType == 'c' || prevNodeType == 'S' || prevNodeType == 's')
                    controlPoint[0] = node + node - controlPoint[1];
                else
                    controlPoint[0] = node;
                REQUIRE(readCoord(controlPoint[1], pathDef));
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 's') {
                    controlPoint[1] += prevNode;
                    node += prevNode;
                }
                contour.addEdge(new CubicSegment(tr(prevNode), tr(controlPoint[0]), tr(controlPoint[1]), tr(node)));
                break;
            case 'A': case 'a':
            {
                Vector2 radius;
                double angle;
                bool largeArg;
                bool sweep;
                REQUIRE(readCoord(radius, pathDef));
                REQUIRE(readDouble(angle, pathDef));
                REQUIRE(readBool(largeArg, pathDef));
                REQUIRE(readBool(sweep, pathDef));
                REQUIRE(readCoord(node, pathDef));
                if (nodeType == 'a')
                    node += prevNode;
                angle *= PI / 180.0;
                addArcApproximate(contour, prevNode, node, radius, angle, largeArg, sweep, tr);
            }
            break;
            default:
                REQUIRE(!"Unknown node type");
            }
            contourStart &= nodeType == 'M' || nodeType == 'm';
            prevNode = node;
            prevNodeType = nodeType;
            readNodeType(nodeType, pathDef);
        }
    NEXT_CONTOUR:
        // Fix contour if it isn't properly closed
        if (!contour.edges.empty() && prevNode != startPoint) {
            if ((contour.edges[contour.edges.size() - 1]->point(1) - contour.edges[0]->point(0)).length() < ENDPOINT_SNAP_RANGE_PROPORTION*size) {
                contour.edges[contour.edges.size() - 1]->moveEndPoint(contour.edges[0]->point(0));
            }
            else {
                contour.addEdge(new LinearSegment(tr(prevNode), tr(startPoint)));
            }
        }
        prevNode = startPoint;
        prevNodeType = '\0';
    }
    return true;
}

struct SVGPath {
    string rawPath;
    XForm transform;
};

SVGRenderResult USVGImportFactory::ImportSVG(vector<uint8>& pixelData, const FString& RawData, FSVGAssetSettings settings)
{
    SVGRenderResult result;
    int32 dimension = enumToDimension(settings.ImportResolution);
    int32 bytesPerPixel = enumToPixelWidth(settings.ImportType);

    vector<SVGPath> paths;
    auto pathCallback = [&paths](const char* path, XForm xform) {
        paths.push_back({ string(path), xform });
    };
    NSVGimage* image = nsvgParse(TCHAR_TO_ANSI(*RawData), "px", 96, pathCallback);
    result.svgWidth = image->width;
    result.svgHeight = image->height;

    // Allocate memory for image
    if (settings.UseSvgResolution) {
        pixelData.resize(image->width * image->height * bytesPerPixel, 0);
    }
    else {
        pixelData.resize(dimension * dimension * bytesPerPixel, 0);
    }

    float requiredSize = FMath::Max(image->width, image->height);
    if (settings.ImportType == ESVGImportType::ESVG_ColorTexture) {
        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (requiredSize > 0) {
            if (settings.UseSvgResolution) {
                nsvgRasterize(rast, image, 0, 0, 1, pixelData.data(), image->width, image->height, image->width * bytesPerPixel);
            }
            else {
                nsvgRasterize(rast, image, 0, 0, dimension / requiredSize, pixelData.data(), dimension, dimension, dimension * bytesPerPixel);
            }
        }
        else {
            result.Problems.Add(TEXT("SVG returned width and height of zero"));
        }
        nsvgDeleteRasterizer(rast);
    }
    else {
        int32 width = settings.UseSvgResolution ? image->width : dimension;
        int32 height = settings.UseSvgResolution ? image->height : dimension;
        float svgScale = settings.UseSvgResolution ? 1 : dimension / image->height;
        Shape dfShape;

        bool useAlternativeRender = settings.ConvertAllElementsToPath;
        if (useAlternativeRender) {
            auto shape = image->shapes;
            while (shape) {
                Contour &contour = dfShape.addContour();
                auto path = shape->paths;
                while (path) {
                    for (int i = 0; i < path->npts - 1; i += 3) {
                        float* p = &path->pts[i * 2];
                        contour.addEdge(new CubicSegment(Point2(svgScale * p[0], height - svgScale * p[1]),
                            Point2(svgScale * p[2], height - svgScale * p[3]),
                            Point2(svgScale * p[4], height - svgScale * p[5]),
                            Point2(svgScale * p[6], height - svgScale * p[7])));
                    }
                    path = path->next;
                }
                shape = shape->next;
            }
        }
        else {
            for (auto path : paths) {
                if (!buildFromPath(dfShape, path.rawPath.c_str(), path.transform, image)) {
                    result.Problems.Add(TEXT("The parser has problems with the SVG's shape descriptions. Try converting all shapes into curve paths."));
                }
            }
        }
        dfShape.normalize();
        if (dfShape.validate()) {
            float scaleFactor = (useAlternativeRender || settings.UseSvgResolution) ? 1 : dimension / requiredSize;
            Vector2 scale(scaleFactor, scaleFactor);
            Vector2 translate(settings.TranslationSDF.X, settings.TranslationSDF.Y);
            double edgeThreshold = settings.EdgeThreshold + 0.00000001;
            edgeColoringSimple(dfShape, settings.AngleThreshold, 0);
            if (settings.ImportType == ESVGImportType::ESVG_MSDF) {
                Bitmap<FloatRGB> msdfOutput(width, height);
                if (useAlternativeRender) {
                    generateMSDF_legacy(msdfOutput, dfShape, settings.Range, scale, translate, edgeThreshold);
                }
                else {
                    generateMSDF(msdfOutput, dfShape, settings.Range, scale, translate, edgeThreshold);
                }
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        FloatRGB pixel = msdfOutput(x, useAlternativeRender ? height - 1 - y : y);
                        int index = x * 4 + y * width * 4;
                        pixelData[index + 0] = FMath::Clamp(int(pixel.b * 0x100), 0, 0xff);
                        pixelData[index + 1] = FMath::Clamp(int(pixel.g * 0x100), 0, 0xff);
                        pixelData[index + 2] = FMath::Clamp(int(pixel.r * 0x100), 0, 0xff);
                        pixelData[index + 3] = 0xff;
                    }
                }
            }
            else if (settings.ImportType == ESVGImportType::ESVG_SDF || settings.ImportType == ESVGImportType::ESVG_PSDF) {
                Bitmap<float> sdfOutput(width, height);
                if (settings.ImportType == ESVGImportType::ESVG_PSDF) {
                    if (useAlternativeRender) {
                        generatePseudoSDF_legacy(sdfOutput, dfShape, settings.Range, scale, translate);
                    }
                    else {
                        generatePseudoSDF(sdfOutput, dfShape, settings.Range, scale, translate);
                    }
                }
                else {
                    if (useAlternativeRender) {
                        generateSDF_legacy(sdfOutput, dfShape, settings.Range, scale, translate);
                    }
                    else {
                        generateSDF(sdfOutput, dfShape, settings.Range, scale, translate);
                    }
                }
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        float pixel = sdfOutput(x, useAlternativeRender ? height - 1 - y : y);
                        int index = x + y * width;
                        pixelData[index] = FMath::Clamp(int(pixel * 0x100), 0, 0xff);
                    }
                }
            }
            else {
                result.Problems.Add(TEXT("Unknown import type"));
            }
        }
        else {
            result.Problems.Add(TEXT("Invalid distance field shape, please check the svg input"));
        }
    }
    nsvgDelete(image);
    return result;
}

bool USVGImportFactory::Reimport(UObject* Obj, const FString& Path)
{
    if (Path.IsEmpty() == false)
    {
        FString FilePath = IFileManager::Get().ConvertToRelativePath(*Path);

        FString Data;
        if (FFileHelper::LoadFileToString(Data, *FilePath))
        {
            const TCHAR* Ptr = *Data;
            CurrentFilename = FilePath; //not thread safe but seems to be how it is done..
            auto Result = FactoryCreateText(Obj->GetClass(), Obj->GetOuter(), Obj->GetFName(), Obj->GetFlags(), NULL, *FPaths::GetExtension(FilePath), Ptr, Ptr + Data.Len(), NULL);
            return true;
        }
    }
    return false;
}

ESVGImportResolution USVGImportFactory::dimensionToEnum(int32 dimension)
{
    if (dimension <= 32) {
        return ESVGImportResolution::ESVG_32;
    }
    else if (dimension <= 64) {
        return ESVGImportResolution::ESVG_64;
    }
    else if (dimension <= 128) {
        return ESVGImportResolution::ESVG_128;
    }
    else if (dimension <= 256) {
        return ESVGImportResolution::ESVG_256;
    }
    else if (dimension <= 512) {
        return ESVGImportResolution::ESVG_512;
    }
    else if (dimension <= 1024) {
        return ESVGImportResolution::ESVG_1024;
    }
    else if (dimension <= 2048) {
        return ESVGImportResolution::ESVG_2048;
    }
    else {
        return ESVGImportResolution::ESVG_4096;
    }
}

int32 USVGImportFactory::enumToDimension(ESVGImportResolution value)
{
    if (value == ESVGImportResolution::ESVG_32) {
        return 32;
    }
    else if (value == ESVGImportResolution::ESVG_64) {
        return 64;
    }
    else if (value == ESVGImportResolution::ESVG_128) {
        return 128;
    }
    else if (value == ESVGImportResolution::ESVG_256) {
        return 256;
    }
    else if (value == ESVGImportResolution::ESVG_512) {
        return 512;
    }
    else if (value == ESVGImportResolution::ESVG_1024) {
        return 1024;
    }
    else if (value == ESVGImportResolution::ESVG_2048) {
        return 2048;
    }
    else if (value == ESVGImportResolution::ESVG_4096) {
        return 4096;
    }
    return 1024;
}

int32 USVGImportFactory::enumToPixelWidth(ESVGImportType value)
{
    if (value == ESVGImportType::ESVG_SDF || value == ESVGImportType::ESVG_PSDF) {
        return 1;
    }
    return 4;
}

ETextureSourceFormat USVGImportFactory::enumToTextureFormat(ESVGImportType value)
{
    if (value == ESVGImportType::ESVG_SDF || value == ESVGImportType::ESVG_PSDF) {
        return ETextureSourceFormat::TSF_G8;
    }
    return ETextureSourceFormat::TSF_BGRA8;
}

#undef LOCTEXT_NAMESPACE

