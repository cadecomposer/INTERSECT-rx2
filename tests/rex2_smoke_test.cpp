// Smoke test for the REX2 import path (VelociLoops -> Rex2Import).
// Usage: rex2_smoke_test <file.rx2> [targetSampleRate]
#include <juce_audio_basics/juce_audio_basics.h>
#include "audio/Rex2Import.h"
#include <cstdio>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "usage: %s file.rx2 [targetSampleRate]\n", argv[0]);
        return 2;
    }

    const juce::File f (argv[1]);
    const double rate = argc > 2 ? juce::String (argv[2]).getDoubleValue() : 48000.0;

    Rex2Import::DecodedLoop loop;
    if (! Rex2Import::decodeFile (f, rate, loop))
    {
        fprintf (stderr, "FAIL: decode of %s\n", f.getFileName().toRawUTF8());
        return 1;
    }

    printf ("OK: %s\n", f.getFileName().toRawUTF8());
    printf ("  tempo      = %.3f bpm\n", loop.tempoBpm);
    printf ("  sampleRate = %.0f Hz\n", loop.sampleRate);
    printf ("  frames     = %d\n", loop.stereo.getNumSamples());
    printf ("  slices     = %zu\n", loop.slices.size());
    for (size_t i = 0; i < loop.slices.size(); ++i)
        printf ("    [%2zu] %7d..%7d (%d frames)\n", i,
                loop.slices[i].startSample, loop.slices[i].endSample,
                loop.slices[i].endSample - loop.slices[i].startSample);

    // Sanity: spans must be contiguous, ordered and in bounds.
    int prev = 0;
    for (const auto& s : loop.slices)
    {
        if (s.startSample < prev || s.endSample > loop.stereo.getNumSamples()
            || s.endSample <= s.startSample)
        {
            fprintf (stderr, "FAIL: bad span %d..%d\n", s.startSample, s.endSample);
            return 1;
        }
        prev = s.endSample;
    }

    // Sanity: buffer must contain non-silent audio.
    float peak = 0.0f;
    for (int i = 0; i < loop.stereo.getNumSamples(); ++i)
        peak = juce::jmax (peak, std::abs (loop.stereo.getSample (0, i)),
                           std::abs (loop.stereo.getSample (1, i)));
    if (peak <= 0.0f)
    {
        fprintf (stderr, "FAIL: silent buffer\n");
        return 1;
    }
    printf ("  peak       = %.4f\n", peak);
    return 0;
}
