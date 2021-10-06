#include "FGLevelModule.h"

AFGLevelModule::AFGLevelModule()
{
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root Scene"));
	RootComponent = RootScene;
}