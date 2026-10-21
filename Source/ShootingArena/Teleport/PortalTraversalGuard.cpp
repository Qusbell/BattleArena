#include "PortalTraversalGuard.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace PortalTraversalGuard
{
	static TMap<TWeakObjectPtr<const AActor>, double> UnlockTimes;

	bool IsLocked(const AActor* actor, double now)
	{
		const double* until = UnlockTimes.Find(actor);
		return until && *until > now;
	}

	void Lock(AActor* actor, double until)
	{
		if (IsValid(actor) && FMath::IsFinite(until)) UnlockTimes.Add(actor, until);
	}

	void Unlock(AActor* actor)
	{
		UnlockTimes.Remove(actor);
	}

	void Cleanup(const UWorld* world, double now)
	{
		for (auto it = UnlockTimes.CreateIterator(); it; ++it)
		{
			const AActor* actor = it.Key().Get();
			// PIE worlds have independent clocks. Never expire another world's valid lock.
			if (!IsValid(actor) || (actor->GetWorld() == world && it.Value() <= now)) it.RemoveCurrent();
		}
	}
}
