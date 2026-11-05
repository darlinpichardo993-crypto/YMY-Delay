// YMY DELAY - BY ORYMY / YMY RECORDS
// Source/DSP/TempoSync.h
//
// Host Tempo Sync: lectura del BPM del host (FL Studio 21/24/25 via VST3),
// divisiones musicales, tiempo libre (FREE), TAP TEMPO y suavizado del
// tiempo de delay sin clicks.
//
// Todo el codigo usado en el hilo de audio es libre de asignaciones y bloqueos.

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>

namespace ymy::dsp
{
//==============================================================================
// Divisiones musicales
//==============================================================================
enum class NoteDivision : int
{
    Whole = 0,                                   // 1/1
    HalfStraight,   HalfDotted,   HalfTriplet,   // 1/2
    QuarterStraight, QuarterDotted, QuarterTriplet,   // 1/4
    EighthStraight, EighthDotted, EighthTriplet,      // 1/8
    SixteenthStraight, SixteenthDotted, SixteenthTriplet, // 1/16
    ThirtySecondStraight, ThirtySecondDotted, ThirtySecondTriplet // 1/32
};

inline constexpr int kNumDivisions = 16;

// Textos para el parametro de tipo Choice (mismo orden que el enum).
inline const juce::StringArray& divisionLabels()
{
    static const juce::StringArray labels {
        "1/1",
        "1/2",  "1/2 Dotted",  "1/2 Triplet",
        "1/4",  "1/4 Dotted",  "1/4 Triplet",
        "1/8",  "1/8 Dotted",  "1/8 Triplet",
        "1/16", "1/16 Dotted", "1/16 Triplet",
        "1/32", "1/32 Dotted", "1/32 Triplet"
    };
    return labels;
}

// Duracion de la division expresada en negras (quarter notes).
inline double divisionInQuarterNotes (NoteDivision d) noexcept
{
    const int i = juce::jlimit (0, kNumDivisions - 1, static_cast<int> (d));

    if (i == 0)
        return 4.0; // 1/1 = 4 negras

    static constexpr double base[] = { 2.0, 1.0, 0.5, 0.25, 0.125 }; // 1/2 ... 1/32
    const int group   = (i - 1) / 3;
    const int variant = (i - 1) % 3; // 0 = recta, 1 = con puntillo, 2 = tresillo

    double q = base[group];
    if (variant == 1) q *= 1.5;
    if (variant == 2) q *= 2.0 / 3.0;
    return q;
}

inline double divisionToMs (NoteDivision d, double bpm) noexcept
{
    return (60000.0 / bpm) * divisionInQuarterNotes (d);
}

//==============================================================================
// Lectura del tempo del host
//==============================================================================
struct TempoInfo
{
    enum class Source { Host, HeldFromHost, Manual };

    double bpm      = 120.0;
    Source source   = Source::Manual;
    bool   playing  = false;
};

class HostTempoReader
{
public:
    static constexpr double kMinBpm = 20.0;
    static constexpr double kMaxBpm = 999.0;

    static bool isValidBpm (double bpm) noexcept
    {
        return std::isfinite (bpm) && bpm >= kMinBpm && bpm <= kMaxBpm;
    }

    void reset() noexcept
    {
        hasHostBpm = false;
        lastHostBpm = 120.0;
    }

    // Llamar UNA vez por bloque en processBlock().
    //  - Host con BPM valido        -> Source::Host
    //  - Host sin dato momentaneo   -> ultimo BPM valido (HeldFromHost)
    //  - Sin host (Standalone, etc) -> BPM manual (parametro / TAP)
    TempoInfo read (juce::AudioPlayHead* playHead, double manualBpm) noexcept
    {
        TempoInfo info;
        info.playing = false;

        if (playHead != nullptr)
        {
            if (auto pos = playHead->getPosition())
            {
                info.playing = pos->getIsPlaying();

                if (auto bpm = pos->getBpm())
                {
                    if (isValidBpm (*bpm))
                    {
                        lastHostBpm = *bpm;
                        hasHostBpm  = true;

                        info.bpm    = *bpm;
                        info.source = TempoInfo::Source::Host;
                        return info;
                    }
                }
            }
        }

        if (hasHostBpm)
        {
            info.bpm    = lastHostBpm;
            info.source = TempoInfo::Source::HeldFromHost;
            return info;
        }

        info.bpm    = isValidBpm (manualBpm) ? manualBpm : 120.0;
        info.source = TempoInfo::Source::Manual;
        return info;
    }

private:
    double lastHostBpm = 120.0;
    bool   hasHostBpm  = false;
};

//==============================================================================
// TAP TEMPO (hilo de mensajes / GUI)
//==============================================================================
class TapTempo
{
public:
    // Devuelve el BPM calculado, o 0.0 si aun no hay suficientes toques.
    double tap() noexcept
    {
        const double now = juce::Time::getMillisecondCounterHiRes();

        if (count > 0 && (now - times[(size_t) (count - 1)]) > kTimeoutMs)
            count = 0; // pausa larga: empezar de nuevo

        if (count == (int) times.size())
        {
            std::rotate (times.begin(), times.begin() + 1, times.end());
            --count;
        }

        times[(size_t) count++] = now;

        if (count < 2)
            return 0.0;

        const double avgInterval = (times[(size_t) (count - 1)] - times[0]) / (double) (count - 1);
        const double bpm = juce::jlimit (HostTempoReader::kMinBpm, 300.0, 60000.0 / avgInterval);
        lastBpm.store (bpm);
        return bpm;
    }

    double getLastBpm() const noexcept { return lastBpm.load(); }

private:
    static constexpr double kTimeoutMs = 2000.0;
    std::array<double, 5> times {};
    int count = 0;
    std::atomic<double> lastBpm { 0.0 };
};

//==============================================================================
// Controlador del tiempo de delay (SYNC / FREE + suavizado)
//==============================================================================
class DelayTimeController
{
public:
    void prepare (double sampleRate, double minMs, double maxMs, double rampMs = 50.0)
    {
        sr = sampleRate;
        minDelayMs = minMs;
        maxDelayMs = maxMs;
        smoothed.reset (sampleRate, rampMs * 0.001);
        firstUpdate = true;
    }

    void reset() noexcept { firstUpdate = true; }

    // Llamar una vez por bloque. Si SYNC esta activo, calcula el tiempo desde
    // el BPM; si no, usa freeMs y no depende del tempo.
    void updateTarget (bool syncOn, NoteDivision division, double bpm, double freeMs) noexcept
    {
        double ms = freeMs;
        folded = false;

        if (syncOn && HostTempoReader::isValidBpm (bpm))
        {
            ms = divisionToMs (division, bpm);

            // El motor llega a maxDelayMs. Si la division no cabe (p. ej. 1/2
            // a 100 BPM), se divide entre 2 hasta que quepa: mantiene la
            // relacion ritmica con la rejilla y evita recortes. Se informa a
            // la GUI mediante wasFolded().
            for (int i = 0; i < 8 && ms > maxDelayMs; ++i) { ms *= 0.5; folded = true; }
            for (int i = 0; i < 8 && ms < minDelayMs; ++i) { ms *= 2.0; folded = true; }
        }

        if (! std::isfinite (ms))
            ms = minDelayMs;

        ms = juce::jlimit (minDelayMs, maxDelayMs, ms);
        timeMsForUI.store (ms);

        const float targetSamples = (float) (ms * 0.001 * sr);

        if (firstUpdate)
        {
            smoothed.setCurrentAndTargetValue (targetSamples);
            lastTarget = targetSamples;
            firstUpdate = false;
        }
        else if (std::abs (targetSamples - lastTarget) > 0.5f) // ignora jitter del BPM
        {
            smoothed.setTargetValue (targetSamples);
            lastTarget = targetSamples;
        }
    }

    // Una vez por muestra: retraso en muestras (fraccionario) con rampa suave.
    float getNextDelaySamples() noexcept { return smoothed.getNextValue(); }

    // Para la GUI (leer desde un Timer, nunca escribir).
    double getTimeMs() const noexcept { return timeMsForUI.load(); }
    bool   wasFolded() const noexcept { return folded; }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothed;
    std::atomic<double> timeMsForUI { 250.0 };
    double sr = 44100.0, minDelayMs = 1.0, maxDelayMs = 500.0;
    float  lastTarget = 0.0f;
    bool   firstUpdate = true;
    bool   folded = false;
};

} // namespace ymy::dsp
