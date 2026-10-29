#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// Rabid FET by Tal Mendel
// A FET-style compressor modelled from the behaviour of the classic design:
//  - fast, program-dependent peak compression (4 / 8 / 12 / 20 : 1 and ALL = rabid mode)
//  - INPUT drives into a fixed threshold, like the hardware
//  - GR-dependent FET gain-cell distortion (more compression = more harmonics)
//  - warm asymmetric class-A style saturation, 4x oversampled
//  - "iron": low-frequency transformer saturation
//  - parallel mix, sidechain high-pass, stereo link
class RabidAudioProcessor : public juce::AudioProcessor
{
public:
    RabidAudioProcessor();
    ~RabidAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Rabid FET"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // read by the editor (meters / visuals)
    std::atomic<float> meterGR { 0.f };   // gain reduction in dB (positive)
    std::atomic<float> inLevel { 0.f };   // linear peak
    std::atomic<float> outLevel { 0.f };  // linear peak

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float delayDry (int ch, float in);

    struct OnePoleHP
    {
        float x1 = 0.f, y1 = 0.f;
        float process (float x, float a)
        {
            const float y = a * (y1 + x - x1);
            x1 = x;
            y1 = y;
            return y;
        }
        void reset() { x1 = 0.f; y1 = 0.f; }
    };

    static constexpr int osFactor = 4;

    double sampleRate = 44100.0;
    int maxBlock = 4096;

    std::array<OnePoleHP, 2> hpA, hpB;
    std::array<float, 2> grEnv {}, grSlow {}, dcX {}, dcY {}, lfState {};
    std::vector<float> grStore;

    juce::SmoothedValue<float> inGain, outGain, mixSm, warmSm;

    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };

    using Duplicator = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Duplicator lowShelf, highShelf;

    juce::AudioBuffer<float> dryBuf;
    std::array<std::vector<float>, 2> dline;
    int dpos = 0, dlen = 0;

    std::atomic<float> *pIn, *pOut, *pAttack, *pRelease, *pRatio, *pWarm, *pIron, *pMix, *pHpf, *pLink;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RabidAudioProcessor)
};
