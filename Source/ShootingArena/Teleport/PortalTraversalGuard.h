#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/** Shared by both portal types, including synchronous overlaps raised during teleport. */
namespace PortalTraversalGuard
{
	bool IsLocked(const AActor* actor, double now);
	void Lock(AActor* actor, double until);
	void Unlock(AActor* actor);
	void Cleanup(const UWorld* world, double now);
}
