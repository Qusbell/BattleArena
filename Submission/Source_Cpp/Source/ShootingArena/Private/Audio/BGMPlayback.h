#pragma once

class UObject;
class USoundBase;

namespace BGMPlayback
{
	// Returns a transient, audio-thread-looping cue for SoundWave/SoundCue tracks.
	// Other sound types must implement looping in their own graph.
	USoundBase* CreateLoopingSound(UObject* Outer, USoundBase* Source);
}
