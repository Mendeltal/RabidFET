#include "PluginEditor.h"

RabidAudioProcessorEditor::RabidAudioProcessorEditor (RabidAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      meter (p), ratio (p),
      ledIn (p.inLevel, "IN"), ledOut (p.outLevel, "OUT"),
      scope (p), sparks (p)
{
    setLookAndFeel (&laf);

    addAndMakeVisible (meter);
    addAndMakeVisible (ratio);
    addAndMakeVisible (ledIn);
    addAndMakeVisible (ledOut);
    addAndMakeVisible (scope);

    setupKnob (input,   "INPUT",   "input",   " dB", 1);
    setupKnob (output,  "OUTPUT",  "output",  " dB", 1);
    setupKnob (attack,  "ATTACK",  "attack",  " us", 0);
    setupKnob (release, "RELEASE", "release", " ms", 0);
    setupKnob (warmth,  "WARMTH",  "warmth",  "",    2);
    setupKnob (iron,    "IRON",    "iron",    "",    2);
    setupKnob (hpf,     "SC HPF",  "scHpf",   " Hz", 0);
    setupKnob (mix,     "MIX",     "mix",     "",    2);

    addAndMakeVisible (link);
    linkAtt = std::make_unique<ButtonAtt> (proc.apvts, "link", link);

    addAndMakeVisible (sparks);   // on top of the meter

    setSize (900, 570);
}

RabidAudioProcessorEditor::~RabidAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void RabidAudioProcessorEditor::setupKnob (Knob& k, const juce::String& name, const char* id,
                                           const juce::String& suffix, int decimals)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 18);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f, true);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setNumDecimalPlacesToDisplay (decimals);
    addAndMakeVisible (k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, rabid::cream.withAlpha (0.85f));
    addAndMakeVisible (k.label);

    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void RabidAudioProcessorEditor::layoutKnob (Knob& k, juce::Rectangle<int> a)
{
    k.label.setBounds (a.removeFromTop (18));
    k.slider.setBounds (a);
}

void RabidAudioProcessorEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (72);

    // main row: input | meter + ratio | output
    auto mainRow = r.removeFromTop (272).reduced (24, 4);
    auto left  = mainRow.removeFromLeft (220);
    auto right = mainRow.removeFromRight (220);
    layoutKnob (input, left);
    layoutKnob (output, right);

    auto mid = mainRow.reduced (6, 0);
    centerPanel = mid.toFloat().expanded (6.f, 2.f);
    sparks.setBounds (mid);
    meter.setBounds (mid.removeFromTop (206));
    mid.removeFromTop (6);
    ratio.setBounds (mid.removeFromTop (56));

    // small knobs
    auto row = r.removeFromTop (124).reduced (24, 2);
    const int w = row.getWidth() / 7;
    Knob* small[] = { &attack, &release, &warmth, &iron, &hpf, &mix };
    for (auto* k : small)
        layoutKnob (*k, row.removeFromLeft (w));
    link.setBounds (row.removeFromLeft (w).reduced (8, 40));

    // bottom bar: LED bars | GR history | signature plate
    auto bar = r.reduced (24, 6);
    barPanel = bar.toFloat();
    auto plate = bar.removeFromRight (264);
    plateRect = plate.toFloat().reduced (4.f, 2.f);

    auto leds = bar.removeFromLeft (290).reduced (10, 8);
    ledIn.setBounds (leds.removeFromTop (28));
    leds.removeFromTop (8);
    ledOut.setBounds (leds.removeFromTop (28));

    scope.setBounds (bar.reduced (10, 6));

    rebuildBackground();
}

void RabidAudioProcessorEditor::rebuildBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;

    using namespace juce;
    const float s = 2.f;   // render at 2x so it stays crisp on retina screens
    bg = Image (Image::ARGB, (int) (w * s), (int) (h * s), true);
    Graphics g (bg);
    g.addTransform (AffineTransform::scale (s));

    const float W = (float) w, H = (float) h;

    // panel
    g.setGradientFill (ColourGradient (rabid::panelTop, 0.f, 0.f, rabid::panelBot, 0.f, H, false));
    g.fillAll();

    // brushed-metal grain
    for (int y = 0; y < h; y += 2)
    {
        g.setColour ((y % 14 == 0 ? Colours::black : Colours::white).withAlpha (y % 14 == 0 ? 0.05f : 0.022f));
        g.drawHorizontalLine (y, 0.f, W);
    }

    // vignette
    g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                       Colours::black.withAlpha (0.6f), 0.f, 0.f, true));
    g.fillAll();

    // inset panels
    g.setColour (Colours::black.withAlpha (0.28f));
    g.fillRoundedRectangle (centerPanel, 16.f);
    g.fillRoundedRectangle (barPanel, 14.f);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (centerPanel, 16.f, 1.f);
    g.drawRoundedRectangle (barPanel, 14.f, 1.f);

    // bevel
    g.setColour (Colours::white.withAlpha (0.18f));
    g.drawLine (1.f, 1.f, W - 1.f, 1.f, 2.f);
    g.drawLine (1.f, 1.f, 1.f, H - 1.f, 2.f);
    g.setColour (Colours::black.withAlpha (0.65f));
    g.drawLine (1.f, H - 1.f, W - 1.f, H - 1.f, 2.f);
    g.drawLine (W - 1.f, 1.f, W - 1.f, H - 1.f, 2.f);

    // title
    g.setFont (Font (FontOptions (42.f, Font::bold)));
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawText ("RABID FET", 38, 14, 360, 48, Justification::centredLeft);
    g.setGradientFill (ColourGradient (Colour (0xfffff4d0), 0.f, 14.f, Colour (0xffe0a040), 0.f, 62.f, false));
    g.drawText ("RABID FET", 36, 12, 360, 48, Justification::centredLeft);

    g.setColour (rabid::cream.withAlpha (0.55f));
    g.setFont (12.f);
    g.drawText ("DISCRETE FET COMPRESSION  /  WARM  /  FEROCIOUS", w - 440, 26, 410, 20, Justification::centredRight);

    g.setGradientFill (ColourGradient (rabid::amber.withAlpha (0.9f), 30.f, 0.f,
                                       rabid::red.withAlpha (0.f), W - 30.f, 0.f, false));
    g.fillRect (30.f, 66.f, W - 60.f, 2.f);

    // screws
    auto screw = [&] (float cx, float cy)
    {
        g.setGradientFill (ColourGradient (Colour (0xffe8e8ee), cx - 5.f, cy - 5.f,
                                           Colour (0xff5a5d66), cx + 5.f, cy + 5.f, false));
        g.fillEllipse (cx - 7.f, cy - 7.f, 14.f, 14.f);
        g.setColour (Colour (0xff2a2c33));
        g.drawLine (cx - 4.f, cy + 1.f, cx + 4.f, cy - 1.f, 1.6f);
    };
    screw (16.f, 16.f);
    screw (W - 16.f, 16.f);
    screw (16.f, H - 16.f);
    screw (W - 16.f, H - 16.f);

    // ---------------------------------------------------------------- signature plate
    auto pr = plateRect;
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (pr.translated (2.f, 4.f), 11.f);
    g.setGradientFill (ColourGradient (rabid::gold1, pr.getX(), pr.getY(),
                                       rabid::gold3, pr.getX(), pr.getBottom(), false));
    g.fillRoundedRectangle (pr, 11.f);

    g.saveState();
    g.reduceClipRegion (pr.toNearestInt());
    for (float y = pr.getY(); y < pr.getBottom(); y += 2.f)
    {
        g.setColour (Colours::white.withAlpha (0.07f));
        g.drawHorizontalLine ((int) y, pr.getX(), pr.getRight());
    }
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.35f), pr.getX(), pr.getY(),
                                       Colours::white.withAlpha (0.f), pr.getCentreX(), pr.getCentreY(), false));
    g.fillRect (pr);
    g.restoreState();

    g.setColour (Colour (0xff5a3a0c).withAlpha (0.75f));
    g.drawRoundedRectangle (pr.reduced (5.f), 8.f, 1.2f);
    g.setColour (Colours::white.withAlpha (0.4f));
    g.drawRoundedRectangle (pr, 11.f, 1.2f);

    g.setColour (Colour (0xff4a2f08));
    g.setFont (9.f);
    g.drawText ("DESIGNED & BUILT BY", pr.withHeight (24.f).toNearestInt(), Justification::centred);

    auto nameArea = pr.withTrimmedTop (12.f).withTrimmedBottom (8.f).toNearestInt();
    g.setFont (rabid::scriptFont (40.f));
    g.setColour (Colours::white.withAlpha (0.55f));                    // engraved highlight
    g.drawText ("Tal Mendel", nameArea.translated (1, 1), Justification::centred);
    g.setColour (Colour (0xff3d2606));
    g.drawText ("Tal Mendel", nameArea, Justification::centred);

    Path flourish;
    const float fx0 = pr.getX() + 30.f, fx1 = pr.getRight() - 30.f, fy = pr.getBottom() - 13.f;
    flourish.startNewSubPath (fx0, fy);
    flourish.cubicTo (fx0 + 50.f, fy + 7.f, fx1 - 90.f, fy - 9.f, fx1, fy - 3.f);
    g.setColour (Colour (0xff3d2606).withAlpha (0.85f));
    g.strokePath (flourish, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

void RabidAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (bg, getLocalBounds().toFloat());
}
