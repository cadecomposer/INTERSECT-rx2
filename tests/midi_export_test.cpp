// Regression test: the slice -> Standard MIDI File export used by drag-and-drop.
//
// Builds a synthetic kit snapshot, writes it through intersectMidi::buildKitMidiFile,
// and parses the result back to verify header, tempo, note timing and that every
// note-on is closed.
#include <juce_audio_processors/juce_audio_processors.h>
#include "midi/MidiExport.h"
#include <cstdio>
#include <vector>

namespace
{
struct ParsedNote
{
    int note = -1;
    int onTick = -1;
    int offTick = -1;
};

bool parseSmf (const uint8_t* data, size_t size, int& tempoUs, std::vector<ParsedNote>& notes)
{
    auto fail = [] (const char* why) { fprintf (stderr, "FAIL: %s\n", why); return false; };

    if (size < 14 || memcmp (data, "MThd", 4) != 0)
        return fail ("missing MThd");

    auto be32 = [] (const uint8_t* p) { return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3]; };
    auto be16 = [] (const uint8_t* p) { return ((uint16_t) p[0] << 8) | p[1]; };

    if (be32 (data + 4) != 6)
        return fail ("bad MThd length");
    if (be16 (data + 8) != 0)
        return fail ("expected format 0");
    if (be16 (data + 10) != 1)
        return fail ("expected 1 track");
    const int division = be16 (data + 12);
    if (division != 960)
        return fail ("expected 960 ticks/quarter");

    if (size < 18 || memcmp (data + 14, "MTrk", 4) != 0)
        return fail ("missing MTrk");
    const size_t trackLen = be32 (data + 18);
    if (size != 14 + 8 + trackLen)
        return fail ("track length mismatch");

    const uint8_t* p = data + 22;
    const uint8_t* end = data + size;
    int tick = 0;
    bool sawTempo = false;

    while (p < end)
    {
        // VLQ delta time
        int delta = 0;
        uint8_t b;
        do
        {
            if (p >= end)
                return fail ("truncated VLQ");
            b = *p++;
            delta = (delta << 7) | (b & 0x7f);
        } while (b & 0x80);
        tick += delta;

        if (p >= end)
            return fail ("truncated status");
        uint8_t status = *p++;

        if (status == 0xFF)   // meta event
        {
            if (p + 2 > end)
                return fail ("truncated meta");
            const uint8_t type = p[0];
            int len = 0;
            b = p[1];
            do
            {
                ++p;
                b = *p++;
                len = (len << 7) | (b & 0x7f);
            } while (b & 0x80);

            if (type == 0x51 && len == 3 && p + 3 <= end)
            {
                tempoUs = (p[0] << 16) | (p[1] << 8) | p[2];
                sawTempo = true;
                p += 3;
            }
            else
            {
                if (p + len > end)
                    return fail ("truncated meta payload");
                p += len;
            }
            continue;
        }

        if ((status & 0xf0) == 0x90)   // note on
        {
            if (p + 2 > end)
                return fail ("truncated note on");
            ParsedNote n;
            n.note = p[0];
            n.onTick = tick;
            notes.push_back (n);
            p += 2;
        }
        else if ((status & 0xf0) == 0x80)   // note off
        {
            if (p + 2 > end)
                return fail ("truncated note off");
            const int note = p[0];
            bool found = false;
            for (auto& n : notes)
                if (n.note == note && n.offTick < 0)
                {
                    n.offTick = tick;
                    found = true;
                    break;
                }
            if (! found)
                return fail ("note off without note on");
            p += 2;
        }
        else
        {
            return fail ("unexpected event in type-0 export");
        }
    }

    if (! sawTempo)
        return fail ("missing tempo meta event");
    for (const auto& n : notes)
        if (n.offTick < 0 || n.offTick <= n.onTick)
            return fail ("unclosed or zero-length note");

    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    constexpr double sampleRate = 44100.0;
    IntersectProcessor::UiSliceSnapshot ui;
    ui.sampleSampleRate = sampleRate;
    ui.numSlices = 3;

    // 120 BPM: 0.1 s == 192 ticks (960 tpq).
    auto& a = ui.slices[0];
    a.active = true;
    a.startSample = 0;
    a.endSample = 4410;     // 0 .. 192 ticks
    a.midiNote = 36;
    a.volume = 0.0f;        // full velocity

    auto& b = ui.slices[1];
    b.active = true;
    b.startSample = 4410;   // 192 .. 384 ticks
    b.endSample = 8820;
    b.midiNote = 37;
    b.volume = -6.02f;      // ~0.5 gain -> velocity ~64

    auto& c = ui.slices[2];
    c.active = true;
    c.startSample = 6615;   // 288 .. 384 ticks, overlaps b
    c.endSample = 8820;
    c.midiNote = 38;

    const auto data = intersectMidi::buildKitMidiFile (ui, 120.0f);
    if (data.getSize() == 0)
    {
        fprintf (stderr, "FAIL: empty midi file\n");
        return 1;
    }

    int tempoUs = -1;
    std::vector<ParsedNote> notes;
    if (! parseSmf (static_cast<const uint8_t*> (data.getData()), data.getSize(), tempoUs, notes))
        return 1;

    if (tempoUs != 500000)
    {
        fprintf (stderr, "FAIL: tempo %d us, expected 500000\n", tempoUs);
        return 1;
    }

    auto findNote = [&] (int note, int onTick, int offTick) -> bool
    {
        for (const auto& n : notes)
            if (n.note == note && n.onTick == onTick && n.offTick == offTick)
                return true;
        return false;
    };

    if (notes.size() != 3 || ! findNote (36, 0, 192) || ! findNote (37, 192, 384) || ! findNote (38, 288, 384))
    {
        fprintf (stderr, "FAIL: unexpected notes (%zu parsed)\n", notes.size());
        for (const auto& n : notes)
            fprintf (stderr, "  note %d on %d off %d\n", n.note, n.onTick, n.offTick);
        return 1;
    }

    // Velocity of the -6 dB slice must be roughly half of the full one.
    // (Re-parsed above only for timing; check raw bytes for velocity.)
    const auto* raw = static_cast<const uint8_t*> (data.getData());
    bool sawHalfVelocity = false, sawFullVelocity = false;
    for (size_t i = 0; i + 2 < data.getSize(); ++i)
        if (raw[i] == 0x90 && raw[i + 1] == 37)
            sawHalfVelocity = raw[i + 2] >= 60 && raw[i + 2] <= 68;
        else if (raw[i] == 0x90 && raw[i + 1] == 36)
            sawFullVelocity = raw[i + 2] >= 125;

    if (! sawHalfVelocity || ! sawFullVelocity)
    {
        fprintf (stderr, "FAIL: velocities (half=%d full=%d)\n", sawHalfVelocity, sawFullVelocity);
        return 1;
    }

    // Empty kit -> empty block.
    IntersectProcessor::UiSliceSnapshot empty;
    if (intersectMidi::buildKitMidiFile (empty, 120.0f).getSize() != 0)
    {
        fprintf (stderr, "FAIL: expected empty file for empty kit\n");
        return 1;
    }

    printf ("PASS\n");
    return 0;
}
