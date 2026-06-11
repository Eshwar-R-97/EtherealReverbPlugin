#include "PluginProcessor.h"
#include "PluginEditor.h"

EtherealReverbProcessor::EtherealReverbProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

EtherealReverbProcessor::~EtherealReverbProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout EtherealReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::preDelay, 1 }, "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 150.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::roomSize, 1 }, "Room Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    // Skew factor 0.4 compresses the upper range — most useful decay times are <5s
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::decay, 1 }, "Decay",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.4f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::damping, 1 }, "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::diffusion, 1 }, "Diffusion",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::modRate, 1 }, "Mod Rate",
        juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::modDepth, 1 }, "Mod Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::tiltEQ, 1 }, "Tilt EQ",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::freeze, 1 }, "Freeze", false));

    return { params.begin(), params.end() };
}

void EtherealReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dspEngine.prepare (sampleRate, samplesPerBlock);
}

void EtherealReverbProcessor::releaseResources()
{
    dspEngine.reset();
}

bool EtherealReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;
    return true;
}

void EtherealReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Assemble the parameter bundle — all atomic loads, safe on the audio thread
    DSPParams params;
    params.preDelay  = apvts.getRawParameterValue (ParamID::preDelay)->load();
    params.roomSize  = apvts.getRawParameterValue (ParamID::roomSize)->load();
    params.decay     = apvts.getRawParameterValue (ParamID::decay)->load();
    params.damping   = apvts.getRawParameterValue (ParamID::damping)->load();
    params.diffusion = apvts.getRawParameterValue (ParamID::diffusion)->load();
    params.modRate   = apvts.getRawParameterValue (ParamID::modRate)->load();
    params.modDepth  = apvts.getRawParameterValue (ParamID::modDepth)->load();
    params.tiltEQ    = apvts.getRawParameterValue (ParamID::tiltEQ)->load();
    params.mix       = apvts.getRawParameterValue (ParamID::mix)->load();
    params.freeze    = apvts.getRawParameterValue (ParamID::freeze)->load() > 0.5f;

    dspEngine.process (buffer, params);
}

juce::AudioProcessorEditor* EtherealReverbProcessor::createEditor()
{
    return new EtherealReverbEditor (*this);
}

void EtherealReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void EtherealReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EtherealReverbProcessor();
}
