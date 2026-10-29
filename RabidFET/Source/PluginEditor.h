#pragma once
#include "PluginProcessor.h"

namespace rabid
{
    static const juce::Colour panelTop { 0xff2f4460 };
    static const juce::Colour panelBot { 0xff0f1622 };
    static const juce::Colour cream    { 0xfff1e3b5 };
    static const juce::Colour amber    { 0xffffb347 };
    static const juce::Colour red      { 0xffff4b3a };
    static const juce::Colour ink      { 0xff0c0e13 };
    static const juce::Colour gold1    { 0xfff7e1a0 };
    static const juce::Colour gold2    { 0xffc9922e };
    static const juce::Colour gold3    { 0xff7a5313 };

    // picks a nice script font that exists on this computer
    inline juce::Font scriptFont (float height)
    {
        const char* candidates[] = { "Snell Roundhand", "Zapfino", "Savoye LET", "Apple Chancery",
                                     "Brush Script MT", "Lucida Handwriting", "Segoe Script", "Bradley Hand" };
        const auto names = juce::Font::findAllTypefaceNames();
        for (auto* c : candidates)
            if (names.contains (c))
                return juce::Font (juce::FontOptions (c, height, juce::Font::plain));
        return juce::Font (juce::FontOptions (height, juce::Font::italic));
    }
}

//==============================================================================
// Hardware look: chrome-ringed black knobs, glowing value arc, LED push buttons
class HardwareLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HardwareLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, rabid::cream);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, rabid::cream);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startA, float endA, juce::Slider&) override
    {
        using namespace juce;
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float d = jmin (b.getWidth(), b.getHeight());
        const auto c = b.getCentre();
        const float r = d * 0.5f;
        const float ang = startA + pos * (endA - startA);

        // tick marks
        g.setColour (rabid::cream.withAlpha (0.55f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startA + (float) i / 10.f * (endA - startA);
            const float s = std::sin (a), co = std::cos (a);
            const float r2 = r * (i % 5 == 0 ? 0.86f : 0.91f);
            g.drawLine (c.x + s * r, c.y - co * r, c.x + s * r2, c.y - co * r2, i % 5 == 0 ? 2.f : 1.2f);
        }

        // glowing value arc
        Path val;
        val.addCentredArc (c.x, c.y, r * 0.80f, r * 0.80f, 0.f, startA, ang, true);
        g.setColour (rabid::amber.withAlpha (0.18f));
        g.strokePath (val, PathStrokeType (9.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (rabid::amber.withAlpha (0.9f));
        g.strokePath (val, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));

        const float rk = r * 0.66f;

        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - rk + 2.f, c.y - rk + 5.f, rk * 2.f, rk * 2.f);

        g.setGradientFill (ColourGradient (Colour (0xfff2f2f5), c.x, c.y - rk,
                                           Colour (0xff5c5f68), c.x, c.y + rk, false));
        g.fillEllipse (c.x - rk, c.y - rk, rk * 2.f, rk * 2.f);

        const float rc = rk * 0.88f;
        g.setGradientFill (ColourGradient (Colour (0xff45454d), c.x, c.y - rc,
                                           Colour (0xff09090c), c.x, c.y + rc, false));
        g.fillEllipse (c.x - rc, c.y - rc, rc * 2.f, rc * 2.f);

        // cap highlight
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.20f), c.x - rc * 0.4f, c.y - rc * 0.7f,
                                           Colours::white.withAlpha (0.f), c.x, c.y, true));
        g.fillEllipse (c.x - rc * 0.8f, c.y - rc * 0.95f, rc * 1.2f, rc * 0.9f);

        Path p;
        p.addRoundedRectangle (-2.2f, -rc * 0.84f, 4.4f, rc * 0.42f, 2.f);
        g.setColour (Colours::white);
        g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn, bool hover, bool) override
    {
        using namespace juce;
        auto b = btn.getLocalBounds().toFloat().reduced (2.f);
        const bool on = btn.getToggleState();

        g.setGradientFill (ColourGradient (Colour (0xff2a2d36), b.getX(), b.getY(),
                                           rabid::ink, b.getX(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 7.f);
        g.setColour (hover ? Colour (0xff5a6275) : Colour (0xff3a4558));
        g.drawRoundedRectangle (b, 7.f, 1.2f);

        const float ledR = 6.f;
        const Point<float> led (b.getX() + 18.f, b.getCentreY());
        if (on)
        {
            g.setColour (rabid::amber.withAlpha (0.3f));
            g.fillEllipse (led.x - ledR - 4.f, led.y - ledR - 4.f, (ledR + 4.f) * 2.f, (ledR + 4.f) * 2.f);
        }
        g.setColour (on ? rabid::amber : Colour (0xff4a3a22));
        g.fillEllipse (led.x - ledR, led.y - ledR, ledR * 2.f, ledR * 2.f);

        g.setColour (rabid::cream);
        g.setFont (14.f);
        g.drawText (btn.getButtonText(), b.withTrimmedLeft (34.f).toNearestInt(), Justification::centredLeft);
    }
};

//==============================================================================
// Gain-reduction meter: backlit face that breathes with the signal, spring needle
class GRMeter : public juce::Component, private juce::Timer
{
public:
    explicit GRMeter (RabidAudioProcessor& p) : proc (p) { startTimerHz (60); }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto b = getLocalBounds().toFloat();
        const float glow = jlimit (0.f, 1.f, 0.30f + lvl * 0.9f);

        g.setColour (rabid::ink);
        g.fillRoundedRectangle (b, 12.f);
        g.setGradientFill (ColourGradient (Colour (0xff566176), b.getX(), b.getY(),
                                           Colour (0xff1c222d), b.getX(), b.getBottom(), false));
        g.drawRoundedRectangle (b.reduced (1.f), 12.f, 2.f);

        auto face = b.reduced (9.f);
        const Colour top = Colour (0xfff7ecc4).interpolatedWith (Colour (0xffffe08a), glow * 0.6f);
        const Colour bot = Colour (0xffd6b868).interpolatedWith (Colour (0xfff2a838), glow * 0.6f);
        g.setGradientFill (ColourGradient (top, face.getCentreX(), face.getY(), bot, face.getCentreX(), face.getBottom(), false));
        g.fillRoundedRectangle (face, 7.f);

        g.saveState();
        g.reduceClipRegion (face.toNearestInt());

        // warm bulbs
        for (float bx : { face.getX() + 22.f, face.getRight() - 22.f })
        {
            g.setGradientFill (ColourGradient (rabid::amber.withAlpha (0.55f * glow), bx, face.getY() + 18.f,
                                               rabid::amber.withAlpha (0.f), bx + 70.f, face.getY() + 18.f, true));
            g.fillEllipse (bx - 70.f, face.getY() - 52.f, 140.f, 140.f);
        }

        const Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.12f);
        const float R = face.getHeight() * 0.95f;

        auto toPos = [&] (float radius, float angle)
        {
            return Point<float> (pivot.x + std::sin (angle) * radius, pivot.y - std::cos (angle) * radius);
        };
        auto angleFor = [] (float db)
        {
            return degreesToRadians (50.f - jlimit (0.f, 24.f, db) / 24.f * 100.f);
        };

        for (int v = 0; v <= 24; ++v)
        {
            const bool major = (v == 0 || v == 2 || v == 4 || v == 7 || v == 10 || v == 20);
            const float a = angleFor ((float) v);
            g.setColour (v >= 10 ? Colour (0xffb3261e) : Colour (0xff2a2418));
            g.drawLine (Line<float> (toPos (R * (major ? 0.78f : 0.82f), a), toPos (R * 0.87f, a)), major ? 2.f : 1.f);
            if (major)
            {
                g.setFont (12.f);
                g.drawText (String (v), Rectangle<float> (28.f, 14.f).withCentre (toPos (R * 0.70f, a)), Justification::centred);
            }
        }

        g.setColour (rabid::ink.withAlpha (0.7f));
        g.setFont (10.f);
        g.drawText ("GAIN REDUCTION", (int) face.getX() + 40, (int) face.getY() + 8, 120, 12, Justification::centredLeft);
        g.drawText ("dB", (int) face.getRight() - 70, (int) face.getY() + 8, 30, 12, Justification::centredRight);

        const float a = angleFor (needle);
        g.setColour (Colours::black.withAlpha (0.22f));
        g.drawLine (Line<float> (toPos (0.f, a).translated (3.f, 3.f), toPos (R * 0.92f, a).translated (3.f, 3.f)), 3.f);
        g.setColour (Colour (0xff15130e));
        g.drawLine (Line<float> (toPos (0.f, a), toPos (R * 0.92f, a)), 2.2f);

        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.24f), face.getCentreX(), face.getY(),
                                           Colours::white.withAlpha (0.f), face.getCentreX(), face.getCentreY(), false));
        g.fillRect (face);

        g.restoreState();
    }

private:
    void timerCallback() override
    {
        const float target = juce::jlimit (0.f, 24.f, proc.meterGR.load());
        vel = vel * 0.62f + (target - needle) * 0.22f;   // slightly underdamped spring
        needle = juce::jlimit (-1.f, 25.f, needle + vel);

        const float l = proc.outLevel.load();
        lvl += (l > lvl ? 0.35f : 0.06f) * (l - lvl);
        repaint();
    }

    RabidAudioProcessor& proc;
    float needle = 0.f, vel = 0.f, lvl = 0.f;
};

//==============================================================================
// Segmented LED level bar with peak hold
class LedBar : public juce::Component, private juce::Timer
{
public:
    LedBar (std::atomic<float>& source, const juce::String& labelText) : src (source), label (labelText)
    {
        startTimerHz (50);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto b = getLocalBounds().toFloat();
        g.setColour (rabid::cream.withAlpha (0.75f));
        g.setFont (11.f);
        g.drawText (label, b.removeFromLeft (34.f), Justification::centredLeft);

        const int N = 28;
        const float segW = b.getWidth() / (float) N;
        const float dbLevel = Decibels::gainToDecibels (level, -60.f);
        const float dbPeak  = Decibels::gainToDecibels (peak, -60.f);

        for (int i = 0; i < N; ++i)
        {
            const float segDb = -48.f + (float) i / (float) (N - 1) * 54.f;   // -48 .. +6 dB
            Colour c = segDb < -18.f ? Colour (0xff3ddc84) : segDb < -6.f ? rabid::amber : rabid::red;
            const bool lit = dbLevel >= segDb;
            const bool isPeak = std::abs (dbPeak - segDb) < (54.f / (float) N) * 0.6f;

            auto r = Rectangle<float> (b.getX() + (float) i * segW, b.getY(), segW, b.getHeight()).reduced (1.2f, 2.f);
            if (lit || isPeak)
            {
                g.setColour (c.withAlpha (0.25f));
                g.fillRoundedRectangle (r.expanded (1.5f), 2.f);
                g.setColour (isPeak && ! lit ? Colours::white.withAlpha (0.9f) : c);
            }
            else
            {
                g.setColour (c.withAlpha (0.12f));
            }
            g.fillRoundedRectangle (r, 1.5f);
        }
    }

private:
    void timerCallback() override
    {
        const float v = src.load();
        level = juce::jmax (v, level * 0.90f);
        if (v >= peak) { peak = v; hold = 40; }
        else if (--hold < 0) peak *= 0.96f;
        repaint();
    }

    std::atomic<float>& src;
    juce::String label;
    float level = 0.f, peak = 0.f;
    int hold = 0;
};

//==============================================================================
// Scrolling history of gain reduction
class GRScope : public juce::Component, private juce::Timer
{
public:
    explicit GRScope (RabidAudioProcessor& p) : proc (p)
    {
        hist.fill (0.f);
        startTimerHz (40);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xff070a10));
        g.fillRoundedRectangle (b, 8.f);
        g.setColour (Colour (0xff2a3446));
        g.drawRoundedRectangle (b.reduced (0.5f), 8.f, 1.2f);

        auto area = b.reduced (6.f, 14.f);
        g.setColour (Colours::white.withAlpha (0.06f));
        for (int i = 1; i <= 3; ++i)
            g.drawHorizontalLine ((int) (area.getY() + area.getHeight() * (float) i / 4.f), area.getX(), area.getRight());

        Path line;
        const int N = (int) hist.size();
        for (int i = 0; i < N; ++i)
        {
            const float v = hist[(size_t) ((head + i) % N)];
            const float x = area.getX() + (float) i / (float) (N - 1) * area.getWidth();
            const float y = area.getY() + jlimit (0.f, 1.f, v / 24.f) * area.getHeight();
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
        }

        Path fill (line);
        fill.lineTo (area.getRight(), area.getY());
        fill.lineTo (area.getX(), area.getY());
        fill.closeSubPath();
        g.setGradientFill (ColourGradient (rabid::amber.withAlpha (0.05f), 0.f, area.getY(),
                                           rabid::red.withAlpha (0.45f), 0.f, area.getBottom(), false));
        g.fillPath (fill);

        g.setColour (rabid::amber.withAlpha (0.3f));
        g.strokePath (line, PathStrokeType (5.f));
        g.setColour (Colour (0xffffe3a8));
        g.strokePath (line, PathStrokeType (1.6f));

        g.setColour (rabid::cream.withAlpha (0.6f));
        g.setFont (10.f);
        g.drawText ("GR HISTORY", b.reduced (8.f, 2.f).removeFromTop (12.f), Justification::centredLeft);
    }

private:
    void timerCallback() override
    {
        hist[(size_t) head] = proc.meterGR.load();
        head = (head + 1) % (int) hist.size();
        repaint();
    }

    RabidAudioProcessor& proc;
    std::array<float, 200> hist;
    int head = 0;
};

//==============================================================================
// Embers that spit off the meter while it is compressing; red when rabid (ALL) is on
class SparkOverlay : public juce::Component, private juce::Timer
{
public:
    explicit SparkOverlay (RabidAudioProcessor& p) : proc (p)
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (40);
    }

    void paint (juce::Graphics& g) override
    {
        for (const auto& s : ps)
        {
            const juce::Colour c = s.hot ? rabid::red : rabid::amber;
            g.setColour (c.withAlpha (s.life * 0.20f));
            g.fillEllipse (s.x - s.size * 2.f, s.y - s.size * 2.f, s.size * 4.f, s.size * 4.f);
            g.setColour (c.brighter (0.5f).withAlpha (s.life * 0.9f));
            g.fillEllipse (s.x - s.size * 0.5f, s.y - s.size * 0.5f, s.size, s.size);
        }
    }

private:
    struct Spark { float x, y, vx, vy, life, size; bool hot; };

    void timerCallback() override
    {
        const float gr = proc.meterGR.load();
        const bool rabidMode = (int) proc.apvts.getRawParameterValue ("ratio")->load() == 4;

        float want = gr / 24.f * 5.f + (rabidMode ? 0.6f : 0.f);
        while (want > 0.f && ps.size() < 260)
        {
            if (rng.nextFloat() < juce::jmin (1.f, want))
            {
                Spark s;
                s.x = (float) getWidth() * (0.22f + 0.56f * rng.nextFloat());
                s.y = (float) getHeight() * 0.42f;
                s.vx = (rng.nextFloat() - 0.5f) * 1.2f;
                s.vy = -(0.6f + rng.nextFloat() * 1.8f);
                s.life = 1.f;
                s.size = 1.5f + rng.nextFloat() * 2.5f;
                s.hot = rabidMode;
                ps.push_back (s);
            }
            want -= 1.f;
        }

        for (auto& s : ps)
        {
            s.x += s.vx;
            s.y += s.vy;
            s.vy *= 0.985f;
            s.life -= 0.03f;
        }
        ps.erase (std::remove_if (ps.begin(), ps.end(), [] (const Spark& s) { return s.life <= 0.f; }), ps.end());
        repaint();
    }

    RabidAudioProcessor& proc;
    std::vector<Spark> ps;
    juce::Random rng;
};

//==============================================================================
// Ratio push buttons: 4 / 8 / 12 / 20 / ALL (ALL = rabid mode)
class RatioButtons : public juce::Component, private juce::Timer
{
public:
    explicit RatioButtons (RabidAudioProcessor& p) : proc (p) { startTimerHz (20); }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const int sel = (int) proc.apvts.getRawParameterValue ("ratio")->load();
        const char* names[5] = { "4", "8", "12", "20", "ALL" };

        auto area = getLocalBounds().toFloat();
        g.setColour (rabid::cream.withAlpha (0.8f));
        g.setFont (11.f);
        g.drawText ("RATIO", area.removeFromTop (14.f), Justification::centred);

        const float w = area.getWidth() / 5.f;
        for (int i = 0; i < 5; ++i)
        {
            auto r = Rectangle<float> (area.getX() + (float) i * w, area.getY(), w, area.getHeight()).reduced (4.f, 2.f);
            const bool on = (i == sel);
            const Colour col = (i == 4) ? rabid::red : rabid::amber;

            if (on)
            {
                g.setColour (col.withAlpha (0.28f));
                g.fillRoundedRectangle (r.expanded (4.f), 9.f);
                g.setGradientFill (ColourGradient (col.brighter (0.2f), r.getX(), r.getY(),
                                                   col.darker (0.4f), r.getX(), r.getBottom(), false));
            }
            else
            {
                g.setGradientFill (ColourGradient (Colour (0xff2a2d36), r.getX(), r.getY(),
                                                   rabid::ink, r.getX(), r.getBottom(), false));
            }
            g.fillRoundedRectangle (r, 6.f);
            g.setColour (on ? col.brighter (0.4f) : Colour (0xff3a4558));
            g.drawRoundedRectangle (r, 6.f, 1.2f);

            g.setColour (on ? rabid::ink : rabid::cream);
            g.setFont (16.f);
            g.drawText (names[i], r, Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto area = getLocalBounds().toFloat().withTrimmedTop (14.f);
        if (! area.contains (e.position)) return;
        const int idx = juce::jlimit (0, 4, (int) (e.position.x / (area.getWidth() / 5.f)));
        if (auto* p = proc.apvts.getParameter ("ratio"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
            p->endChangeGesture();
        }
    }

private:
    void timerCallback() override { repaint(); }
    RabidAudioProcessor& proc;
};

//==============================================================================
class RabidAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RabidAudioProcessorEditor (RabidAudioProcessor&);
    ~RabidAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };

    void setupKnob (Knob&, const juce::String& name, const char* id, const juce::String& suffix, int decimals);
    static void layoutKnob (Knob&, juce::Rectangle<int>);
    void rebuildBackground();

    RabidAudioProcessor& proc;
    HardwareLookAndFeel laf;

    GRMeter meter;
    RatioButtons ratio;
    LedBar ledIn, ledOut;
    GRScope scope;
    SparkOverlay sparks;

    Knob input, output, attack, release, warmth, iron, hpf, mix;
    juce::ToggleButton link { "LINK" };
    std::unique_ptr<ButtonAtt> linkAtt;

    juce::Image bg;
    juce::Rectangle<float> centerPanel, barPanel, plateRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RabidAudioProcessorEditor)
};
