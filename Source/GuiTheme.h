#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace HNStudioTheme
{
    // Neon Cyber Dark Palette
    static const juce::Colour bgDark           { 0xff0a0d14 };
    static const juce::Colour bgPanel          { 0xff101622 };
    static const juce::Colour bgCard           { 0xff161f30 };
    static const juce::Colour bgCardHighlight  { 0xff1e2942 };
    static const juce::Colour borderSubtle     { 0xff25334e };

    static const juce::Colour neonCyan         { 0xff00f0ff };
    static const juce::Colour neonGreen        { 0xff00ff88 };
    static const juce::Colour neonMagenta      { 0xffff007f };
    static const juce::Colour neonPurple       { 0xffa855f7 };
    static const juce::Colour neonAmber        { 0xfff59e0b };
    static const juce::Colour neonRed          { 0xffff3366 };

    static const juce::Colour textBright       { 0xfff8fafc };
    static const juce::Colour textSecondary    { 0xff94a3b8 };
    static const juce::Colour textDim          { 0xff64748b };

    class NeonLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        NeonLookAndFeel()
        {
            setColour(juce::ResizableWindow::backgroundColourId, bgDark);
            setColour(juce::TextButton::buttonColourId, bgCard);
            setColour(juce::TextButton::buttonOnColourId, neonCyan.withAlpha(0.25f));
            setColour(juce::TextButton::textColourOffId, textBright);
            setColour(juce::TextButton::textColourOnId, neonCyan);
            
            setColour(juce::Slider::thumbColourId, neonCyan);
            setColour(juce::Slider::trackColourId, bgCardHighlight);
            setColour(juce::Slider::rotarySliderFillColourId, neonCyan);
            setColour(juce::Slider::rotarySliderOutlineColourId, borderSubtle);

            setColour(juce::ComboBox::backgroundColourId, bgCard);
            setColour(juce::ComboBox::textColourId, textBright);
            setColour(juce::ComboBox::outlineColourId, borderSubtle);

            setColour(juce::PopupMenu::backgroundColourId, bgPanel);
            setColour(juce::PopupMenu::textColourId, textBright);
            setColour(juce::PopupMenu::highlightedBackgroundColourId, neonCyan.withAlpha(0.2f));
        }

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider& slider) override
        {
            auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
            auto centreX = (float)x + (float)width  * 0.5f;
            auto centreY = (float)y + (float)height * 0.5f;
            auto rx = centreX - radius;
            auto ry = centreY - radius;
            auto rw = radius * 2.0f;
            auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // Background circle
            g.setColour(bgCardHighlight);
            g.fillEllipse(rx, ry, rw, rw);

            g.setColour(borderSubtle);
            g.drawEllipse(rx, ry, rw, rw, 1.5f);

            // Arc track
            juce::Path p;
            p.addCentredArc(centreX, centreY, radius - 4.0f, radius - 4.0f, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(neonCyan);
            g.strokePath(p, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Indicator dot
            juce::Path dot;
            auto dotRadius = 3.5f;
            auto dotDistance = radius - 8.0f;
            auto dotX = centreX + dotDistance * std::sin(angle);
            auto dotY = centreY - dotDistance * std::cos(angle);
            dot.addEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
            
            g.setColour(textBright);
            g.fillPath(dot);
        }

        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            juce::ignoreUnused(minSliderPos, maxSliderPos);
            
            if (slider.isVertical())
            {
                auto trackWidth = 6.0f;
                auto trackX = (float)x + ((float)width - trackWidth) * 0.5f;
                
                // Track background
                g.setColour(borderSubtle);
                g.fillRoundedRectangle(trackX, (float)y, trackWidth, (float)height, 3.0f);

                // Active track
                g.setColour(neonCyan);
                g.fillRoundedRectangle(trackX, sliderPos, trackWidth, (float)(y + height) - sliderPos, 3.0f);

                // Thumb
                auto thumbWidth = 20.0f;
                auto thumbHeight = 10.0f;
                auto thumbX = (float)x + ((float)width - thumbWidth) * 0.5f;
                
                g.setColour(bgCardHighlight);
                g.fillRoundedRectangle(thumbX, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight, 3.0f);

                g.setColour(neonCyan);
                g.drawRoundedRectangle(thumbX, sliderPos - thumbHeight * 0.5f, thumbWidth, thumbHeight, 3.0f, 1.5f);
            }
            else
            {
                juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            }
        }
    };
}
