#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout RabidAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addFloat = [&] (const char* id, const char* name, NormalisableRange<float> r, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def));
    };

    addFloat ("input",   "Input",    { 0.f, 48.f, 0.01f }, 14.f);
    addFloat ("output",  "Output",   { -30.f, 12.f, 0.01f }, -10.f);
    addFloat ("attack",  "Attack",   { 20.f, 800.f, 0.1f, 0.45f }, 150.f);
    addFloat ("release", "Release",  { 50.f, 1100.f, 0.1f, 0.5f }, 250.f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ratio", 1 }, "Ratio",
                                                         StringArray { "4", "8", "12", "20", "ALL" }, 0));
    addFloat ("warmth",  "Warmth",   { 0.f, 1.f, 0.001f }, 0.35f);
    addFloat ("iron",    "Iron",     { 0.f, 1.f, 0.001f }, 0.45f);
    addFloat ("mix",     "Mix",      { 0.f, 1.f, 0.001f }, 1.f);
    addFloat ("scHpf",   "SC HPF",   { 20.f, 500.f, 0.1f, 0.5f }, 60.f);
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 }, "Link", true));

    return { p.begin(), p.end() };
}

RabidAudioProcessor::RabidAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pIn      = apvts.getRawParameterValue ("input");
    pOut     = apvts.getRawParameterValue ("output");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pRatio   = apvts.getRawParameterValue ("ratio");
    pWarm    = apvts.getRawParameterValue ("warmth");
    pIron    = apvts.getRawParameterValue ("iron");
    pMix     = apvts.getRawParameterValue ("mix");
    pHpf     = apvts.getRawParameterValue ("scHpf");
    pLink    = apvts.getRawParameterValue ("link");
}

bool RabidAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void RabidAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    maxBlock = juce::jmax (samplesPerBlock, 512);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };

    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();
    const int latency = (int) std::lround (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    dlen = latency;
    for (auto& d : dline) d.assign ((size_t) juce::jmax (1, dlen), 0.f);
    dpos = 0;
    dryBuf.setSize (2, maxBlock);
    grStore.assign ((size_t) maxBlock, 0.f);

    lowShelf.prepare (spec);
    highShelf.prepare (spec);
    *lowShelf.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sr, 90.f, 0.7f, juce::Decibels::decibelsToGain (1.6f));
    *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 9000.f, 0.6f, juce::Decibels::decibelsToGain (-0.8f));
    lowShelf.reset();
    highShelf.reset();

    for (auto& f : hpA) f.reset();
    for (auto& f : hpB) f.reset();
    grEnv.fill (0.f); grSlow.fill (0.f); dcX.fill (0.f); dcY.fill (0.f); lfState.fill (0.f);

    inGain.reset  (sr, 0.03); inGain.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.reset (sr, 0.03); outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    mixSm.reset   (sr, 0.03); mixSm.setCurrentAndTargetValue   (pMix->load());
    warmSm.reset  (sr, 0.05); warmSm.setCurrentAndTargetValue  (pWarm->load());
}

float RabidAudioProcessor::delayDry (int ch, float in)
{
    if (dlen <= 0) return in;
    auto& line = dline[(size_t) ch];
    const float out = line[(size_t) dpos];
    line[(size_t) dpos] = in;
    return out;
}

// threshold sits at 0 dBFS (internal); the INPUT knob pushes the signal into it
static inline float computeGR (float levelDb, float ratio, float knee)
{
    const float slope = 1.f - 1.f / ratio;
    if (levelDb <= -knee * 0.5f) return 0.f;
    if (levelDb >= knee * 0.5f)  return slope * levelDb;
    const float t = levelDb + knee * 0.5f;
    return slope * t * t / (2.f * knee);
}

void RabidAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) return;

    const int   ratioIdx  = juce::jlimit (0, 4, (int) pRatio->load());
    const bool  rabid     = ratioIdx == 4;
    static constexpr float ratios[5] = { 4.f, 8.f, 12.f, 20.f, 20.f };
    const float ratio     = ratios[ratioIdx];
    const float knee      = rabid ? 2.f : 6.f;
    const float grScale   = rabid ? 1.25f : 1.f;
    const bool  link      = pLink->load() > 0.5f;
    const float iron      = juce::jlimit (0.f, 1.f, pIron->load() + (rabid ? 0.2f : 0.f));

    const float sr = (float) sampleRate;
    const float attackSec   = pAttack->load() * 1.0e-6f * (rabid ? 1.8f : 1.f);
    const float attackCoef  = std::exp (-1.f / (attackSec * sr));
    const float releaseBase = pRelease->load() * 1.0e-3f * (rabid ? 0.7f : 1.f);
    const float hpCoef      = std::exp (-juce::MathConstants<float>::twoPi * pHpf->load() / sr);
    const float slowCoef    = 1.f - std::exp (-1.f / (0.3f * sr));
    const float lfCoef      = 1.f - std::exp (-juce::MathConstants<float>::twoPi * 140.f / sr);

    inGain.setTargetValue  (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    mixSm.setTargetValue   (pMix->load());
    warmSm.setTargetValue  (juce::jlimit (0.f, 1.f, pWarm->load() + (rabid ? 0.25f : 0.f)));

    const int total = buffer.getNumSamples();
    juce::dsp::AudioBlock<float> full (buffer);
    float maxGR = 0.f, peakIn = 0.f, peakOut = 0.f;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        float* x[2]  = { buffer.getWritePointer (0) + start, buffer.getWritePointer (1) + start };
        float* dr[2] = { dryBuf.getWritePointer (0), dryBuf.getWritePointer (1) };

        // ---------------- compressor (sample by sample)
        for (int i = 0; i < n; ++i)
        {
            peakIn = juce::jmax (peakIn, std::abs (x[0][i]), std::abs (x[1][i]));

            const float g = inGain.getNextValue();
            float xin[2], det[2];

            for (int ch = 0; ch < 2; ++ch)
            {
                dr[ch][i] = delayDry (ch, x[ch][i]);
                xin[ch] = x[ch][i] * g;
                det[ch] = std::abs (hpB[(size_t) ch].process (hpA[(size_t) ch].process (xin[ch], hpCoef), hpCoef));
            }
            if (dlen > 0) dpos = (dpos + 1) % dlen;

            if (link)
            {
                const float m = juce::jmax (det[0], det[1]);
                det[0] = det[1] = m;
            }

            for (int ch = 0; ch < 2; ++ch)
            {
                const float lvl    = juce::Decibels::gainToDecibels (det[ch], -120.f);
                const float target = computeGR (lvl, ratio, knee) * grScale;

                float& env  = grEnv[(size_t) ch];
                float& slow = grSlow[(size_t) ch];

                if (target > env)
                {
                    env += (1.f - attackCoef) * (target - env);
                }
                else
                {
                    // program-dependent release: sustained heavy GR releases slower
                    const float stress  = juce::jlimit (0.f, 1.f, slow / 10.f);
                    const float relTime = releaseBase * (0.35f + 1.25f * stress);
                    const float relCoef = std::exp (-1.f / (relTime * sr));
                    env = relCoef * env + (1.f - relCoef) * target;
                }
                slow += slowCoef * (env - slow);
                maxGR = juce::jmax (maxGR, env);

                x[ch][i] = xin[ch] * juce::Decibels::decibelsToGain (-env);
            }
            grStore[(size_t) i] = juce::jmax (grEnv[0], grEnv[1]);
        }

        auto chunk = full.getSubsetChannelBlock (0, 2).getSubBlock ((size_t) start, (size_t) n);

        // ---------------- FET gain-cell distortion + warm saturation, 4x oversampled
        {
            const float W = warmSm.getNextValue();
            warmSm.skip (n - 1);
            const float kk = 1.f + 2.f * W;
            const float bias = 0.12f * W;
            const float biasOff = std::tanh (kk * bias);
            constexpr float ref = 2.f;

            auto up = oversampling.processSamplesUp (chunk);
            const size_t upN = up.getNumSamples();

            for (size_t ch = 0; ch < 2; ++ch)
            {
                float* p = up.getChannelPointer (ch);
                for (size_t j = 0; j < upN; ++j)
                {
                    // interpolate the gain reduction up to the oversampled rate
                    const float pos = (float) j / (float) osFactor;
                    const int i0 = juce::jmin ((int) pos, n - 1);
                    const int i1 = juce::jmin (i0 + 1, n - 1);
                    const float fr = pos - (float) i0;
                    const float gr = grStore[(size_t) i0] * (1.f - fr) + grStore[(size_t) i1] * fr;

                    // the more it compresses, the more the FET gain cell colours the sound
                    const float depth = juce::jlimit (0.f, 1.5f, gr / 12.f);
                    const float a2 = 0.06f * depth * (0.4f + W);
                    const float a3 = 0.05f * depth * (0.4f + W);

                    float xn = p[j] / ref;
                    xn = xn + a2 * xn * xn - a3 * xn * xn * xn;
                    p[j] = ref * (std::tanh (kk * (xn + bias)) - biasOff) / kk;
                }
            }
            oversampling.processSamplesDown (chunk);
        }

        // ---------------- iron: transformer-style low-frequency saturation
        if (iron > 0.001f)
        {
            const float d = 1.f + 3.f * iron;
            for (int ch = 0; ch < 2; ++ch)
            {
                float& lf = lfState[(size_t) ch];
                for (int i = 0; i < n; ++i)
                {
                    const float in = x[ch][i];
                    lf += lfCoef * (in - lf);
                    const float hi = in - lf;
                    const float satLf = std::tanh (d * lf / 1.2f) * 1.2f / d;
                    x[ch][i] = hi + lf + iron * (satLf - lf);
                }
            }
        }

        // ---------------- DC blocker (the asymmetry adds a little offset)
        for (int ch = 0; ch < 2; ++ch)
        {
            float& px = dcX[(size_t) ch];
            float& py = dcY[(size_t) ch];
            for (int i = 0; i < n; ++i)
            {
                const float in = x[ch][i];
                const float y = in - px + 0.9995f * py;
                px = in;
                py = y;
                x[ch][i] = y;
            }
        }

        // ---------------- gentle low bump + soft top
        {
            juce::dsp::ProcessContextReplacing<float> ctx (chunk);
            lowShelf.process (ctx);
            highShelf.process (ctx);
        }

        // ---------------- output gain + parallel mix
        for (int i = 0; i < n; ++i)
        {
            const float og = outGain.getNextValue();
            const float m  = mixSm.getNextValue();
            for (int ch = 0; ch < 2; ++ch)
            {
                x[ch][i] = dr[ch][i] * (1.f - m) + x[ch][i] * og * m;
                peakOut = juce::jmax (peakOut, std::abs (x[ch][i]));
            }
        }
    }

    meterGR.store (maxGR);
    inLevel.store (peakIn);
    outLevel.store (peakOut);
}

juce::AudioProcessorEditor* RabidAudioProcessor::createEditor()
{
    return new RabidAudioProcessorEditor (*this);
}

void RabidAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void RabidAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RabidAudioProcessor();
}
