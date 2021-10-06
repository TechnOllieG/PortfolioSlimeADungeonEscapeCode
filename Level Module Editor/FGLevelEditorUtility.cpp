#include "FGLevelEditorUtility.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "SlimePlatformer/Map Features/FGModuleWall.h"
#include "SlimePlatformer/Systems/Procedural Generation/FGLevelModule.h"
#include "EditorAssetLibrary.h"
#include "SlimePlatformer/Map Features/FGModuleObstacle.h"
#include "SlimePlatformer/Systems/Procedural Generation/FGLevelDataSpecs.h"
#include "SlimePlatformer/Systems/Procedural Generation/FGProceduralGenerator.h"

DEFINE_LOG_CATEGORY(LogLevelEditorUtility);

void UFGLevelEditorUtility::CreateLevelModules(TArray<UTexture2D*> LevelSchematics, UFGLevelDataSpecs* DataSpecs, UWorld* World)
{
	TArray<AFGLevelModule*> LevelModules;
	for(int i = 0; i < LevelSchematics.Num(); i++)
	{
		UWorld* CurrentWorld = World;
		if(World == nullptr)
			World = GEditor->GetEditorWorldContext().World();
		
		LevelModules.Add(World->SpawnActor<AFGLevelModule>(AFGLevelModule::StaticClass()));
	}
	AddComponentsToActors(LevelModules, LevelSchematics, DataSpecs);
}

void UFGLevelEditorUtility::AddComponentsToActors(TArray<AFGLevelModule*> EmptyLevelModuleActors, TArray<UTexture2D*> LevelSchematics, UFGLevelDataSpecs* DataSpecs)
{
#if WITH_EDITOR
	UE_LOG(LogLevelEditorUtility, Log, TEXT("Tried to create level modules"));
	
	int ModuleSize = DataSpecs->ModuleSize;
	for (int i = 0; i < LevelSchematics.Num(); i++)
	{
		UTexture2D* Texture = LevelSchematics[i];
		AFGLevelModule* LevelModuleInstance = EmptyLevelModuleActors[i];
		
		FString Name = FString(TEXT("BP_") + Texture->GetName());
		FString PackageName = FString(TEXT("/Game/_Game/Blueprints/LevelModules/")) + Name;
		if (UEditorAssetLibrary::DoesAssetExist(PackageName))
		{
			UE_LOG(LogLevelEditorUtility, Error, TEXT("Couldn't create a blueprint with name %s since it already exists, please delete it first"), *Name);
			continue;
		}
		if(ModuleSize != Texture->PlatformData->Mips[0].SizeX || ModuleSize != Texture->PlatformData->Mips[0].SizeY)
		{
			UE_LOG(LogLevelEditorUtility, Error, TEXT("Couldn't create a blueprint with name %s since the resolution of the texture doesn't match the module size defined in LevelDataSpecs"), *Name)
			continue;
		}
		
		ParseTexture(LevelModuleInstance, Texture, DataSpecs);

		FKismetEditorUtilities::FCreateBlueprintFromActorParams Params;
		Params.bOpenBlueprint = false;
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprintFromActor(PackageName, LevelModuleInstance, Params);

		if(GEditor)
		{
			GEditor->GetEditorWorldContext().World()->EditorDestroyActor(LevelModuleInstance, true);
		}
	}
#endif
}

void UFGLevelEditorUtility::ParseTexture(AFGLevelModule* LevelModule, UTexture2D* Texture, UFGLevelDataSpecs* DataSpecs)
{
	FTexture2DMipMap Mip = Texture->PlatformData->Mips[0];
	FByteBulkData BulkData = Mip.BulkData;
	FColor* ImageData = static_cast<FColor*>(BulkData.Lock(LOCK_READ_ONLY));

	const int ModuleSize = DataSpecs->ModuleSize;
	const float TileSize = DataSpecs->TileSize;
	const int CorridorWidth = DataSpecs->CorridorWidth;

	const int MiddleIndex = ModuleSize * 0.5f;
	
	for(int y = 0; y < Mip.SizeY; y++)
	{
		for(int x = 0; x < Mip.SizeX; x++)
		{
			const FVector RelativePos = FVector(0.f, (x - MiddleIndex) * TileSize, (MiddleIndex - y) * TileSize);
			ParsePixelColor(LevelModule, ImageData[y * Mip.SizeX + x], RelativePos, DataSpecs, x, y);
		}
	}
	BulkData.Unlock();
}

void UFGLevelEditorUtility::ParsePixelColor(AFGLevelModule* LevelModule, FColor PixelColor, FVector RelativePosition, UFGLevelDataSpecs* DataSpecs, int xCoord, int yCoord)
{
	const int ModuleSize = DataSpecs->ModuleSize;

	if(FMath::Abs(PixelColor.A) <= ColorTolerance)
	{
		if(LevelModule->DoorLocations[Up] == -1 && yCoord == 0)
			LevelModule->DoorLocations[Up] = xCoord;
		
		if(LevelModule->DoorLocations[Left] == -1 && xCoord == 0)
			LevelModule->DoorLocations[Left] = yCoord;
		
		if(LevelModule->DoorLocations[Down] == -1 && yCoord == ModuleSize - 1)
			LevelModule->DoorLocations[Down] = xCoord;
		
		if(LevelModule->DoorLocations[Right] == -1 && xCoord == ModuleSize - 1)
			LevelModule->DoorLocations[Right] = yCoord;

		return;
	}
	
	for(FColorMappingData Set : DataSpecs->ColorMappings)
	{
		if(!Set.Class && !Set.ActorClass)
			continue;
		
		if(!ColorsAreAlmostEqual(Set.Color, PixelColor))
			continue;

		USceneComponent* Component;
		if(Set.ClassTypeToUse.GetValue() == ActorComponent)
		{
			Component = NewObject<USceneComponent>(LevelModule, Set.Class, FName(TEXT("Feature_") + FString::FromInt(xCoord) + TEXT(",") + FString::FromInt(yCoord)), RF_Transactional);
		}
		else
		{
			Component = NewObject<UChildActorComponent>(LevelModule, UChildActorComponent::StaticClass(), FName(TEXT("Feature_") + FString::FromInt(xCoord) + TEXT(",") + FString::FromInt(yCoord)), RF_Transactional);
			Cast<UChildActorComponent>(Component)->SetChildActorClass(Set.ActorClass);
		}

		LevelModule->AddInstanceComponent(Component);
		Component->AttachToComponent(LevelModule->GetRootComponent(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		Component->SetRelativeLocation(RelativePosition);
	}
}

bool UFGLevelEditorUtility::ColorsAreAlmostEqual(const FColor A, const FColor B, const int Tolerance)
{
	return FMath::Abs(A.R - B.R) <= Tolerance && FMath::Abs(A.G - B.G) <= Tolerance && FMath::Abs(A.B - B.B) <= Tolerance && FMath::Abs(A.A - B.A) <= Tolerance;
}
