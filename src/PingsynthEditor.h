#pragma once
/*
 * AUTO GENERATED,
 * NOT A GOOD IDEA TO CHANGE STUFF HERE
 * Keep the file readonly
 */

#include "PingsynthProcessor.h"

#include "UiElements.h"

//==============================================================================
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor, juce::Timer
{
  public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
        : AudioProcessorEditor(&p)
        , processorRef(p)
        , valueTreeState(vts)
        , backgroundApp(juce::Colour(Constants::Colors::bg_App))
    {
        setLookAndFeel(&m_laf);
        initWidgets();
        setResizable(true, true);
        setResizeLimits(Constants::InitJuce::WindowWidth, Constants::InitJuce::WindowHeight, 4000, 3000);
        setSize(Constants::InitJuce::WindowWidth, Constants::InitJuce::WindowHeight);
        startTimerHz(Constants::InitJuce::TimerHertz);
    }

    ~AudioPluginAudioProcessorEditor() override
    {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(backgroundApp);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
    void resized() override
    {
        auto area = getLocalBounds().reduced(static_cast<int>(Constants::Margins::big));

        // auto generated
        // const juce::FlexItem::Margin knobMargin = juce::FlexItem::Margin(Constants::Margins::small);
        const juce::FlexItem::Margin knobMarginSmall = juce::FlexItem::Margin(Constants::Margins::medium);
        std::vector<juce::Rectangle<int>> areas(4);
        const auto colWidth = area.getWidth() / 11;
        areas[0] = area.removeFromLeft(colWidth * 1).reduced(Constants::Margins::small);
        areas[1] = area.removeFromLeft(colWidth * 2).reduced(Constants::Margins::small);
        areas[2] = area.removeFromLeft(colWidth * 2).reduced(Constants::Margins::small);
        areas[3] = area.reduced(Constants::Margins::small);

        {
            juce::FlexBox box;
            box.flexWrap = juce::FlexBox::Wrap::noWrap;
            box.flexDirection = juce::FlexBox::Direction::column;
            box.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
            box.items.add(juce::FlexItem(volDial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(reverbLevelDial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(levelGauge).withHeight(200).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(cpuGauge).withHeight(200).withMargin(knobMarginSmall));
            box.performLayout(areas[0].toFloat());
        }
        {
            juce::FlexBox box;
            box.flexWrap = juce::FlexBox::Wrap::noWrap;
            box.flexDirection = juce::FlexBox::Direction::column;
            box.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
            box.items.add(juce::FlexItem(user1Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user2Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user3Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user4Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user5Dial).withFlex(1).withMargin(knobMarginSmall));
            box.performLayout(areas[1].toFloat());
        }
        {
            juce::FlexBox box;
            box.flexWrap = juce::FlexBox::Wrap::noWrap;
            box.flexDirection = juce::FlexBox::Direction::column;
            box.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
            box.items.add(juce::FlexItem(user6Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user7Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user8Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user9Dial).withFlex(1).withMargin(knobMarginSmall));
            box.items.add(juce::FlexItem(user10Dial).withFlex(1).withMargin(knobMarginSmall));
            box.performLayout(areas[2].toFloat());
        }
        {
            juce::FlexBox box;
            box.flexWrap = juce::FlexBox::Wrap::noWrap;
            box.flexDirection = juce::FlexBox::Direction::column;
            box.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
            box.items.add(juce::FlexItem(spectrogramGauge).withFlex(1).withMargin(knobMarginSmall));
            box.performLayout(areas[3].toFloat());
        }
    }
#pragma GCC diagnostic pop

    void timerCallback() override
    {
        cpuGauge.update(processorRef.getCpuLoad());
        levelGauge.update(processorRef.getInputDbLoad(), processorRef.getOutputDbLoad());
        spectrogramGauge.update(processorRef.getSpectrogram());
    }

    void initWidgets()
    {
        addAndMakeVisible(volDial);
        volDial.reset(valueTreeState, "vol");
        volDial.setLabelText(juce::String::fromUTF8("Vol"));
        addAndMakeVisible(reverbLevelDial);
        reverbLevelDial.reset(valueTreeState, "reverbLevel");
        reverbLevelDial.setLabelText(juce::String::fromUTF8("Reverb"));
        addAndMakeVisible(user1Dial);
        user1Dial.reset(valueTreeState, "user1");
        user1Dial.setLabelText(juce::String::fromUTF8("Decay"));
        addAndMakeVisible(user2Dial);
        user2Dial.reset(valueTreeState, "user2");
        user2Dial.setLabelText(juce::String::fromUTF8("Spread"));
        addAndMakeVisible(user3Dial);
        user3Dial.reset(valueTreeState, "user3");
        user3Dial.setLabelText(juce::String::fromUTF8("Odds"));
        addAndMakeVisible(user4Dial);
        user4Dial.reset(valueTreeState, "user4");
        user4Dial.setLabelText(juce::String::fromUTF8("Evens"));
        addAndMakeVisible(user5Dial);
        user5Dial.reset(valueTreeState, "user5");
        user5Dial.setLabelText(juce::String::fromUTF8("Odds Skew"));
        addAndMakeVisible(user6Dial);
        user6Dial.reset(valueTreeState, "user6");
        user6Dial.setLabelText(juce::String::fromUTF8("Evens Skew"));
        addAndMakeVisible(user7Dial);
        user7Dial.reset(valueTreeState, "user7");
        user7Dial.setLabelText(juce::String::fromUTF8("Piano Stretch"));
        addAndMakeVisible(user8Dial);
        user8Dial.reset(valueTreeState, "user8");
        user8Dial.setLabelText(juce::String::fromUTF8("Rand Spread"));
        addAndMakeVisible(user9Dial);
        user9Dial.reset(valueTreeState, "user9");
        user9Dial.setLabelText(juce::String::fromUTF8("Rand Power"));
        addAndMakeVisible(user10Dial);
        user10Dial.reset(valueTreeState, "user10");
        user10Dial.setLabelText(juce::String::fromUTF8("Ctrl 8"));
        addAndMakeVisible(cpuGauge);
        cpuGauge.setLabelText(juce::String::fromUTF8("CPU"));
        addAndMakeVisible(levelGauge);
        levelGauge.setLabelText(juce::String::fromUTF8("Level"));
        addAndMakeVisible(spectrogramGauge);
        spectrogramGauge.setLabelText(juce::String::fromUTF8("Spectrogram"));
    }

  private:
    AudioPluginAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& valueTreeState;
    GuiLookAndFeel m_laf;
    juce::Colour backgroundApp;

    CustomRotaryDial volDial{this};
    CustomRotaryDial reverbLevelDial{this};
    CustomRotaryDial user1Dial{this};
    CustomRotaryDial user2Dial{this};
    CustomRotaryDial user3Dial{this};
    CustomRotaryDial user4Dial{this};
    CustomRotaryDial user5Dial{this};
    CustomRotaryDial user6Dial{this};
    CustomRotaryDial user7Dial{this};
    CustomRotaryDial user8Dial{this};
    CustomRotaryDial user9Dial{this};
    CustomRotaryDial user10Dial{this};
    CpuGauge cpuGauge{};
    Gauge levelGauge{};
    SpectrogramDisplay spectrogramGauge{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
