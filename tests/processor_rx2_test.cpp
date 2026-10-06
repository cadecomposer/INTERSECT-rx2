// Regression test: full processor load path for REX2 files.
//
// Reproduces the use-after-move crash where processBlock read
// decoded->importedSlices after applyDecodedSample(std::move(decoded)):
//   #0 std::vector<ImportedSlice>::empty()
//   #1 IntersectProcessor::processBlock(...)
//
// Usage: processor_rx2_test <file.rx2>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <cstdio>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "usage: %s file.rx2\n", argv[0]);
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    IntersectProcessor proc;
    const double rate = 48000.0;
    proc.prepareToPlay (rate, 512);

    proc.loadFileAsync (juce::File (argv[1]));

    // Pump the audio thread and message loop until the decode job lands.
    // The load is applied inside processBlock — this is where the old crash hit.
    bool loaded = false;
    for (int i = 0; i < 4000 && ! loaded; ++i)
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        proc.processBlock (buf, midi);
        juce::Thread::sleep (2);
        loaded = proc.sampleData.isLoaded();
    }

    if (! loaded)
    {
        fprintf (stderr, "FAIL: sample never loaded\n");
        return 1;
    }

    const int numSlices = proc.sliceManager.getNumSlices();
    printf ("loaded: %d frames @ %.0f Hz\n",
            proc.sampleData.getNumFrames(), proc.sampleData.getDecodedSampleRate());
    printf ("slices: %d, imported tempo: %.3f bpm\n",
            numSlices, proc.sampleData.getImportedTempoBpm());

    for (int i = 0; i < numSlices; ++i)
    {
        const Slice& s = proc.sliceManager.getSlice (i);
        printf ("  [%2d] note=%3d %7d..%7d bpm=%.1f\n",
                i, s.midiNote, s.startSample, s.endSample, s.bpm);
    }

    if (numSlices == 0)
    {
        fprintf (stderr, "FAIL: no slices created from REX2 metadata\n");
        return 1;
    }

    // Slices must be in order and inside the buffer.
    const int frames = proc.sampleData.getNumFrames();
    for (int i = 0; i < numSlices; ++i)
    {
        const Slice& s = proc.sliceManager.getSlice (i);
        if (s.startSample < 0 || s.endSample > frames || s.endSample <= s.startSample)
        {
            fprintf (stderr, "FAIL: slice %d out of bounds (%d..%d of %d)\n",
                     i, s.startSample, s.endSample, frames);
            return 1;
        }
        if (i > 0 && s.startSample < proc.sliceManager.getSlice (i - 1).startSample)
        {
            fprintf (stderr, "FAIL: slices out of order at %d\n", i);
            return 1;
        }
    }

    // A few more blocks after the load — catches late use-after-free.
    for (int i = 0; i < 100; ++i)
    {
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        proc.processBlock (buf, midi);
    }

    printf ("PASS\n");
    return 0;
}
