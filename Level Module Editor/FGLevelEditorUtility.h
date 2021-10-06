#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FGLevelEditorUtility.generated.h"

#define ColorTolerance 8

class UFGLevelDataSpecs;
class AFGLevelModule;
DECLARE_LOG_CATEGORY_EXTERN(LogLevelEditorUtility, Log, All);

UCLASS()
class SLIMEPLATFORMEREDITOR_API UFGLevelEditorUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void CreateLevelModules(TArray<UTexture2D*> LevelSchematics, UFGLevelDataSpecs* DataSpecs, UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable)
	static void AddComponentsToActors(TArray<AFGLevelModule*> EmptyLevelModuleActors, TArray<UTexture2D*> LevelSchematics, UFGLevelDataSpecs* DataSpecs);

private:
	static void ParseTexture(AFGLevelModule* LevelModule, UTexture2D* Texture, UFGLevelDataSpecs* DataSpecs);
	static void ParsePixelColor(AFGLevelModule* LevelModule, FColor PixelColor, FVector RelativePosition, UFGLevelDataSpecs* DataSpecs, int xCoord, int yCoord);
	static bool ColorsAreAlmostEqual(const FColor A, const FColor B, const int Tolerance = ColorTolerance);
};