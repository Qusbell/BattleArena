#include "Audio/BGMPlayback.h"

#include "Sound/SoundCue.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectGlobals.h"

USoundBase* BGMPlayback::CreateLoopingSound(UObject* Outer, USoundBase* Source)
{
	USoundCue* PlaybackCue = nullptr;
	if (USoundCue* SourceCue = Cast<USoundCue>(Source))
	{
		if (!SourceCue->FirstNode) return Source;
		TArray<USoundNode*> Nodes;
		SourceCue->RecursiveFindAllNodes(SourceCue->FirstNode, Nodes);
		for (USoundNode* Node : Nodes)
		{
			const USoundNodeWavePlayer* Player = Cast<USoundNodeWavePlayer>(Node);
			if (Player && Player->GetSoundWave() && Player->GetSoundWave()->bProcedural) return Source;
		}

		// Duplicate the cue and its owned graph, NOT the externally referenced waves.
		// Editing shared asset nodes here would also change SFX using that asset.
		FObjectDuplicationParameters Parameters(SourceCue, Outer);
		Parameters.ApplyFlags = RF_Transient;
		Parameters.FlagMask &= ~(RF_Public | RF_Standalone | RF_Transactional);
		PlaybackCue = CastChecked<USoundCue>(StaticDuplicateObjectEx(Parameters));
	}
	else if (USoundWave* SourceWave = Cast<USoundWave>(Source))
	{
		// MetaSoundSource also derives from SoundWave. Procedural generators need
		// their own loop graph; wrapping them cannot loop a finite PCM buffer.
		if (SourceWave->bProcedural) return Source;
		PlaybackCue = NewObject<USoundCue>(Outer, NAME_None, RF_Transient);
		// Neutral cue gain: the wave and the track still supply their original gain.
		PlaybackCue->VolumeMultiplier = 1.0f;
		PlaybackCue->PitchMultiplier = 1.0f;
		PlaybackCue->SoundClassObject = SourceWave->SoundClassObject;
		PlaybackCue->VirtualizationMode = SourceWave->VirtualizationMode;
		PlaybackCue->Priority = SourceWave->Priority;
		PlaybackCue->bBypassVolumeScaleForPriority = SourceWave->bBypassVolumeScaleForPriority;
		PlaybackCue->bOverrideConcurrency = SourceWave->bOverrideConcurrency;
		PlaybackCue->ConcurrencySet = SourceWave->ConcurrencySet;
		PlaybackCue->ConcurrencyOverrides = SourceWave->ConcurrencyOverrides;
		PlaybackCue->SoundSubmixObject = SourceWave->SoundSubmixObject;
		PlaybackCue->SoundSubmixSends = SourceWave->SoundSubmixSends;
		PlaybackCue->SourceEffectChain = SourceWave->SourceEffectChain;
		PlaybackCue->bEnableBaseSubmix = SourceWave->bEnableBaseSubmix;
		PlaybackCue->bEnableSubmixSends = SourceWave->bEnableSubmixSends;
		PlaybackCue->bEnableBusSends = SourceWave->bEnableBusSends;
		PlaybackCue->BusSends = SourceWave->BusSends;
		PlaybackCue->PreEffectBusSends = SourceWave->PreEffectBusSends;
		USoundNodeWavePlayer* WavePlayer = NewObject<USoundNodeWavePlayer>(PlaybackCue, NAME_None, RF_Transient);
		WavePlayer->SetSoundWave(SourceWave);
		PlaybackCue->FirstNode = WavePlayer;
	}
	else
	{
		return Source;
	}

	if (USoundNodeWavePlayer* WavePlayer = Cast<USoundNodeWavePlayer>(PlaybackCue->FirstNode))
	{
		// A single-wave cue can use seamless native looping.
		WavePlayer->bLooping = true;
	}
	else
	{
		// Repeat the whole graph (including sequences/random choices), not each leaf.
		USoundNodeLooping* LoopNode = NewObject<USoundNodeLooping>(PlaybackCue, NAME_None, RF_Transient);
		LoopNode->bLoopIndefinitely = true;
		LoopNode->ChildNodes.Add(PlaybackCue->FirstNode);
		PlaybackCue->FirstNode = LoopNode;
	}

	// CacheAggregateValues only recalculates Duration in the editor. Set it explicitly
	// as well so packaged builds classify the transient cue as a looping sound.
	PlaybackCue->Duration = INDEFINITELY_LOOPING_DURATION;
	PlaybackCue->CacheAggregateValues();
	return PlaybackCue;
}
