// YMY DELAY - BY ORYMY / YMY RECORDS
// Source/DSP/DelayEngine.h
//
// Motor DSP: delay corto con interpolacion Hermite, feedback con tono y saturacion,
// HP/LP, modulacion sutil, Mono/Estereo/Ping-Pong, ducking, width y mezcla.
// Sin asignaciones de memoria ni bloqueos en process().

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "TempoSync.h"

namespace ymy::dsp
{
enum class DelayMode : int { Mono = 0, Stereo = 1, PingPong = 2 };

struct EngineParams
{
    bool      on        = true;
    float     feedback  = 0.25f;  // 0..0.9
    float     mix       = 0.25f;  // 0..1
    float     tone      = 0.5f;   // 0..1
    float     width     = 1.0f;   // 0..2
    float     duck      = 0.0f;   // 0..1
    DelayMode mode      = DelayMode::Stereo;
    bool      analog    = true;
    bool      filterOn  = true;
    float     hpHz      = 120.0f;
    float     lpHz      = 7000.0f;
    float     outputDb  = 0.0f;
};

//==============================================================================
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        int size = 1;
        while (size < maxSamples + 8)
            size <<= 1;

        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    int size() const noexcept { return (int) buffer.size(); }

    void push (float x) noexcept
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    // delay en muestras (>= 1) contado desde la ultima muestra escrita. Hermite de 4 puntos.
    float read (float delay) const noexcept
    {
        const int   i0 = (int) delay;
        const float t  = delay - (float) i0;

        const float xm1 = at (i0 - 1);
        const float x0  = at (i0);
        const float x1  = at (i0 + 1);
        const float x2  = at (i0 + 2);

        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);

        return ((c3 * t + c2) * t + c1) * t + c0;
    }

private:
    float at (int k) const noexcept { return buffer[(size_t) ((writePos - 1 - k) & mask)]; }

    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
};

//==============================================================================
class DelayEngine
{
public:
    static constexpr double kMinDelayMs = 1.0;
    static constexpr double kMaxDelayMs = 500.0;

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;

        const int maxSamples = (int) std::ceil ((kMaxDelayMs + 2.0) * 0.001 * sr) + 16;
        lineL.prepare (maxSamples);
        lineR.prepare (maxSamples);

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) std::max (1, maxBlockSize), 2 };
        hp.prepare (spec);
        lp.prepare (spec);
        hp.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        lp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

        constexpr double ramp = 0.03;
        for (auto* s : { &fbS, &mixS, &widthS, &duckS, &outS, &filterS, &analogS })
            s->reset (sampleRate, ramp);
        onS.reset (sampleRate, 0.02);
        for (auto* s : { &hpS, &lpS, &toneS })
            s->reset (sampleRate, ramp);

        atkCoef = 1.0f - std::exp (-1.0f / (0.005f * (float) sr));
        relCoef = 1.0f - std::exp (-1.0f / (0.150f * (float) sr));

        reset();
    }

    void reset() noexcept
    {
        lineL.clear();
        lineR.clear();
        hp.reset();
        lp.reset();
        toneL = toneR = 0.0f;
        env = 0.0f;
        lfoPhase = 0.0;
        needsSnap = true;
    }

    float getLastPeakL() const noexcept { return lastPeakL; }
    float getLastPeakR() const noexcept { return lastPeakR; }

    void process (juce::AudioBuffer<float>& buffer, const EngineParams& p, DelayTimeController& time) noexcept
    {
        const int n   = buffer.getNumSamples();
        const int nch = juce::jmin (2, buffer.getNumChannels());
        if (n <= 0 || nch <= 0)
            return;

        setTargets (p);

        float* chL = buffer.getWritePointer (0);
        float* chR = nch > 1 ? buffer.getWritePointer (1) : nullptr;
        const bool stereoOut = (chR != nullptr);

        const bool mono     = (p.mode == DelayMode::Mono);
        const bool pingPong = (p.mode == DelayMode::PingPong);

        const float  modDepthSamples = (float) (kModDepthMs * 0.001 * sr);
        const double lfoInc          = kModRateHz / sr;
        const float  maxD            = (float) lineL.size() - 4.0f;
        const float  twoPi           = juce::MathConstants<float>::twoPi;

        float peakL = 0.0f, peakR = 0.0f;

        int pos = 0;
        while (pos < n)
        {
            const int len = std::min (kSubBlock, n - pos);

            // Coeficientes de filtros: se actualizan por sub-bloque con valores suavizados.
            hp.setCutoffFrequency (hpS.skip (len));
            lp.setCutoffFrequency (std::min (lpS.skip (len), (float) (0.45 * sr)));
            const float toneHz = toneS.skip (len);
            const float toneA  = std::exp (-twoPi * toneHz / (float) sr);

            for (int i = pos; i < pos + len; ++i)
            {
                const float fb     = fbS.getNextValue();
                const float mix    = mixS.getNextValue();
                const float width  = widthS.getNextValue();
                const float duck   = duckS.getNextValue();
                const float outG   = outS.getNextValue();
                const float on     = onS.getNextValue();
                const float fm     = filterS.getNextValue();
                const float an     = analogS.getNextValue();
                const float d      = time.getNextDelaySamples();

                // Modulacion sutil (solo con ANALOG), fases distintas en L y R.
                const float phase = (float) lfoPhase;
                const float modL  = an * modDepthSamples * std::sin (twoPi * phase);
                const float modR  = an * modDepthSamples * std::sin (twoPi * (phase + 0.25f));
                lfoPhase += lfoInc;
                if (lfoPhase >= 1.0) lfoPhase -= 1.0;

                const float dL = juce::jlimit (2.0f, maxD, d + modL);
                const float dR = mono ? dL : juce::jlimit (2.0f, maxD, d + modR);

                const float inL0 = finiteOrZero (chL[i]);
                const float inR0 = stereoOut ? finiteOrZero (chR[i]) : inL0;
                const float inL  = mono ? 0.5f * (inL0 + inR0) : inL0;
                const float inR  = mono ? inL : inR0;

                // Detector de ducking sobre la senal seca.
                const float lvl = std::max (std::abs (inL0), std::abs (inR0));
                env += (lvl > env ? atkCoef : relCoef) * (lvl - env);

                // Lectura (read-before-push: se resta 1 para que el retardo sea exacto).
                const float rawL = lineL.read (dL - 1.0f);
                const float rawR = lineR.read (dR - 1.0f);

                // TONE: pasa-bajos suave; cada repeticion vuelve a pasar por el, asi se oscurece.
                toneL = (1.0f - toneA) * rawL + toneA * toneL;
                toneR = (1.0f - toneA) * rawR + toneA * toneR;

                // Filtros HP/LP sobre la salida humeda (siempre corriendo; FILTER hace crossfade).
                const float fL = lp.processSample (0, hp.processSample (0, toneL));
                const float fR = lp.processSample (1, hp.processSample (1, toneR));
                const float wL = toneL + fm * (fL - toneL);
                const float wR = toneR + fm * (fR - toneR);

                // Escritura en las lineas (entrada + feedback).
                float wrL, wrR;
                if (pingPong)
                {
                    const float x = 0.5f * (inL0 + inR0);
                    wrL = x + fb * saturate (toneR, an);
                    wrR = fb * saturate (toneL, an);
                }
                else
                {
                    wrL = inL + fb * saturate (toneL, an);
                    wrR = inR + fb * saturate (toneR, an);
                }

                lineL.push (sanitize (wrL));
                lineR.push (sanitize (wrR));

                // Ducking y width sobre la senal humeda.
                const float duckGain = 1.0f - duck * 0.9f * juce::jlimit (0.0f, 1.0f, env * 4.0f);
                float oL = wL * duckGain;
                float oR = wR * duckGain;

                if (stereoOut && ! mono)
                {
                    const float m = 0.5f * (oL + oR);
                    const float s = 0.5f * (oL - oR) * width;
                    oL = m + s;
                    oR = m - s;
                }

                const float dryG = 1.0f - mix;
                const float wetG = mix;

                const float procL = (inL0 * dryG + oL * wetG) * outG;
                const float outL  = inL0 + on * (procL - inL0);
                chL[i] = outL;
                peakL = std::max (peakL, std::abs (outL));

                if (stereoOut)
                {
                    const float procR = (inR0 * dryG + oR * wetG) * outG;
                    const float outR  = inR0 + on * (procR - inR0);
                    chR[i] = outR;
                    peakR = std::max (peakR, std::abs (outR));
                }
            }

            pos += len;
        }

        lastPeakL = peakL;
        lastPeakR = stereoOut ? peakR : peakL;
    }

private:
    static constexpr int    kSubBlock    = 32;
    static constexpr double kModDepthMs  = 0.35;
    static constexpr double kModRateHz   = 0.8;

    using Lin = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    using Mul = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;

    static float finiteOrZero (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }

    static float sanitize (float x) noexcept
    {
        return std::isfinite (x) ? juce::jlimit (-4.0f, 4.0f, x) : 0.0f;
    }

    // Saturacion suave tipo cinta; con amount = 0 es transparente.
    static float saturate (float x, float amount) noexcept
    {
        constexpr float drive = 1.6f;
        const float s = std::tanh (x * drive) / drive;
        return x + amount * (s - x);
    }

    static float toneToHz (float tone) noexcept
    {
        return 1200.0f * std::pow (10.0f, juce::jlimit (0.0f, 1.0f, tone)); // 1.2 kHz .. 12 kHz
    }

    void setTargets (const EngineParams& p) noexcept
    {
        auto set = [this] (auto& s, float v)
        {
            if (needsSnap) s.setCurrentAndTargetValue (v);
            else           s.setTargetValue (v);
        };

        set (fbS,     juce::jlimit (0.0f, 0.9f, p.feedback));
        set (mixS,    juce::jlimit (0.0f, 1.0f, p.mix));
        set (widthS,  juce::jlimit (0.0f, 2.0f, p.width));
        set (duckS,   juce::jlimit (0.0f, 1.0f, p.duck));
        set (outS,    juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 12.0f, p.outputDb)));
        set (onS,     p.on ? 1.0f : 0.0f);
        set (filterS, p.filterOn ? 1.0f : 0.0f);
        set (analogS, p.analog ? 1.0f : 0.0f);
        set (hpS,     juce::jlimit (20.0f, 1000.0f, p.hpHz));
        set (lpS,     juce::jlimit (1000.0f, 20000.0f, p.lpHz));
        set (toneS,   toneToHz (p.tone));

        needsSnap = false;
    }

    double sr = 44100.0;

    DelayLine lineL, lineR;
    juce::dsp::StateVariableTPTFilter<float> hp, lp;

    Lin fbS, mixS, widthS, duckS, outS, onS, filterS, analogS;
    Mul hpS, lpS, toneS;

    float toneL = 0.0f, toneR = 0.0f;
    float env = 0.0f, atkCoef = 0.01f, relCoef = 0.001f;
    double lfoPhase = 0.0;
    bool needsSnap = true;

    float lastPeakL = 0.0f, lastPeakR = 0.0f;
};
} // namespace ymy::dsp
