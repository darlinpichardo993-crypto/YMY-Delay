// YMY DELAY - BY ORYMY / YMY RECORDS
// Estilo visual: carbon + dorado, perillas grandes, botones tipo pildora.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ymy::ui
{
inline const juce::Colour kGold    { 0xffc9a24b };
inline const juce::Colour kGoldDim { 0xff8a7234 };
inline const juce::Colour kBg      { 0xff121214 };
inline const juce::Colour kPanel   { 0xff0d0d0f };
inline const juce::Colour kText    { 0xffe8e2d0 };

inline juce::Font font (float height, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (height, style));
}

class YMYLookAndFeel : public juce::LookAndFeel_V4
{
public:
    YMYLookAndFeel()
    {
        using juce::Colour;
        setColour (juce::ResizableWindow::backgroundColourId, kBg);
        setColour (juce::Label::textColourId, kText);

        setColour (juce::Slider::textBoxTextColourId, kText);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::thumbColourId, kGold);
        setColour (juce::Slider::trackColourId, kGoldDim);
        setColour (juce::Slider::backgroundColourId, Colour (0xff2a2a2d));

        setColour (juce::ComboBox::backgroundColourId, Colour (0xff1c1c1f));
        setColour (juce::ComboBox::textColourId, kText);
        setColour (juce::ComboBox::outlineColourId, kGoldDim);
        setColour (juce::ComboBox::arrowColourId, kGold);

        setColour (juce::PopupMenu::backgroundColourId, Colour (0xff18181b));
        setColour (juce::PopupMenu::textColourId, kText);
        setColour (juce::PopupMenu::headerTextColourId, kGold);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kGoldDim);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);

        setColour (juce::TextButton::buttonColourId, Colour (0xff202024));
        setColour (juce::TextButton::textColourOffId, kText);
        setColour (juce::TextButton::textColourOnId, kText);

        setColour (juce::AlertWindow::backgroundColourId, Colour (0xff18181b));
        setColour (juce::AlertWindow::textColourId, kText);
        setColour (juce::AlertWindow::outlineColourId, kGoldDim);

        setColour (juce::TextEditor::backgroundColourId, Colour (0xff0d0d0f));
        setColour (juce::TextEditor::textColourId, kText);
        setColour (juce::TextEditor::outlineColourId, kGoldDim);
        setColour (juce::TextEditor::focusedOutlineColourId, kGold);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.0f); }
    juce::Font getPopupMenuFont() override { return font (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (12.0f, juce::Font::bold); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float alpha = slider.isEnabled() ? 1.0f : 0.35f;

        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        const float arcR = radius - 3.0f;
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff2a2a2d));
        g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (kGold.withAlpha (alpha));
        g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = radius * 0.68f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3d42), centre.x, centre.y - kr,
                                                 juce::Colour (0xff131315), centre.x, centre.y + kr, false));
        g.fillEllipse (centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (kGoldDim.withAlpha (0.6f * alpha));
        g.drawEllipse (centre.x - kr, centre.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

        juce::Path pointer;
        pointer.addRoundedRectangle (-2.0f, -kr + 3.0f, 4.0f, kr * 0.42f, 1.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (kGold.withAlpha (alpha));
        g.fillPath (pointer);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        const float alpha = b.isEnabled() ? 1.0f : 0.4f;

        g.setColour ((on ? kGold.withAlpha (0.92f) : juce::Colour (0xff1c1c1f)).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kGold.withAlpha ((on ? 1.0f : 0.45f) * alpha));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);

        g.setColour ((on ? juce::Colour (0xff0d0d0f) : kText).withMultipliedAlpha (alpha));
        g.setFont (font (12.5f, juce::Font::bold));
        g.drawText (b.getButtonText(), r.toNearestInt(), juce::Justification::centred, false);
    }
};
} // namespace ymy::ui
