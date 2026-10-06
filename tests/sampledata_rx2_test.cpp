// Integration test: SampleData::decodeFromFiles with REX2 files.
// Usage: sampledata_rx2_test <file.rx2|wav> [more files...] [targetSampleRate]
#include <juce_audio_formats/juce_audio_formats.h>
#include "audio/SampleData.h"
#include <cstdio>

int main (int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf (stderr, "usage: %s file1 [file2 ...] targetSampleRate\n", argv[0]);
        return 2;
    }

    std::vector<juce::File> files;
    for (int i = 1; i < argc - 1; ++i)
        files.push_back (juce::File (argv[i]));
    const double rate = juce::String (argv[argc - 1]).getDoubleValue();

    auto decoded = SampleData::decodeFromFiles (files, rate);
    if (decoded == nullptr)
    {
        fprintf (stderr, "FAIL: decodeFromFiles\n");
        return 1;
    }

    printf ("OK: %zu file(s)\n", files.size());
    printf ("  buffer frames = %d @ %.0f Hz\n", decoded->decodedNumFrames, decoded->decodedSampleRate);
    printf ("  session samples = %zu\n", decoded->sessionSamples.size());
    for (const auto& s : decoded->sessionSamples)
        printf ("    id=%d %s [%d..%d]\n", s.sampleId, s.fileName.toRawUTF8(),
                s.startFrame, s.startFrame + s.numFrames);
    printf ("  imported slices = %zu, tempo = %.3f bpm\n",
            decoded->importedSlices.size(), decoded->importedTempoBpm);

    // Every imported slice must lie inside the buffer and inside a session sample.
    for (const auto& imp : decoded->importedSlices)
    {
        if (imp.startSample < 0 || imp.endSample > decoded->decodedNumFrames
            || imp.endSample <= imp.startSample)
        {
            fprintf (stderr, "FAIL: imported slice out of bounds %d..%d\n",
                     imp.startSample, imp.endSample);
            return 1;
        }

        bool inRegion = false;
        for (const auto& s : decoded->sessionSamples)
            if (imp.startSample >= s.startFrame && imp.endSample <= s.startFrame + s.numFrames)
                inRegion = true;
        if (! inRegion)
        {
            fprintf (stderr, "FAIL: imported slice %d..%d not inside any session sample\n",
                     imp.startSample, imp.endSample);
            return 1;
        }
    }

    printf ("PASS\n");
    return 0;
}
