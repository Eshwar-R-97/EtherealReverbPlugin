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

    // Skew 0.35 keeps short decays accessible; max 60s for extreme ambient tails
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::decay, 1 }, "Decay",
        juce::NormalisableRange<float> (0.1f, 60.0f, 0.01f, 0.35f), 2.0f));

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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::decayColor, 1 }, "Decay Color",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmer, 1 }, "Shimmer",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmerPitch, 1 }, "Shimmer Pitch",
        juce::NormalisableRange<float> (0.5f, 3.0f, 0.01f), 2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmerChar, 1 }, "Shimmer Character",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmerShiftHz, 1 }, "Shimmer Shift Hz",
        juce::NormalisableRange<float> (5.0f, 50.0f, 0.5f), 15.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamID::shimmerVoices, 1 }, "Shimmer Voices",
        juce::StringArray { "1", "2", "3", "4", "5" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::freeze, 1 }, "Freeze", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::reverse, 1 }, "Reverse", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::reverseMix, 1 }, "Reverse Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::dream, 1 }, "Dream",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::swim, 1 }, "Swim",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::modShape, 1 }, "Mod Shape",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmerDrift, 1 }, "Shimmer Drift",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::shimmerDir, 1 }, "Shimmer Dir",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::glow, 1 }, "Glow",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::cloud, 1 }, "Cloud",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::vox, 1 }, "Vox",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::realm, 1 }, "Realm",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::chaos, 1 }, "Chaos",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

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
    auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono())
        return false;
    return true;
}

void EtherealReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Mono input (e.g. standalone mic): duplicate channel 0 into channel 1
    if (buffer.getNumChannels() == 1)
        buffer.setSize (2, buffer.getNumSamples(), true, true, true);
    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    // Assemble the parameter bundle — all atomic loads, safe on the audio thread
    DSPParams params;
    params.preDelay  = apvts.getRawParameterValue (ParamID::preDelay)->load();
    params.roomSize  = apvts.getRawParameterValue (ParamID::roomSize)->load();
    params.decay     = apvts.getRawParameterValue (ParamID::decay)->load();
    params.damping   = apvts.getRawParameterValue (ParamID::damping)->load();
    params.diffusion = apvts.getRawParameterValue (ParamID::diffusion)->load();
    params.modRate   = apvts.getRawParameterValue (ParamID::modRate)->load();
    params.modDepth  = apvts.getRawParameterValue (ParamID::modDepth)->load();
    params.tiltEQ     = apvts.getRawParameterValue (ParamID::tiltEQ)->load();
    params.mix        = apvts.getRawParameterValue (ParamID::mix)->load();
    params.decayColor     = apvts.getRawParameterValue (ParamID::decayColor)->load();
    params.shimmer        = apvts.getRawParameterValue (ParamID::shimmer)->load();
    params.shimmerPitch   = apvts.getRawParameterValue (ParamID::shimmerPitch)->load();
    params.shimmerChar    = apvts.getRawParameterValue (ParamID::shimmerChar)->load();
    params.shimmerShiftHz = apvts.getRawParameterValue (ParamID::shimmerShiftHz)->load();
    params.shimmerVoices  = (int) apvts.getRawParameterValue (ParamID::shimmerVoices)->load() + 1;
    params.freeze         = apvts.getRawParameterValue (ParamID::freeze)->load() > 0.5f;
    params.reverse        = apvts.getRawParameterValue (ParamID::reverse)->load() > 0.5f;
    params.reverseMix     = apvts.getRawParameterValue (ParamID::reverseMix)->load();
    params.dream          = apvts.getRawParameterValue (ParamID::dream)->load();
    params.swim           = apvts.getRawParameterValue (ParamID::swim)->load();
    params.modShape       = apvts.getRawParameterValue (ParamID::modShape)->load();
    params.shimmerDrift   = apvts.getRawParameterValue (ParamID::shimmerDrift)->load();
    params.shimmerDir     = apvts.getRawParameterValue (ParamID::shimmerDir)->load();
    params.glow           = apvts.getRawParameterValue (ParamID::glow)->load();
    params.cloud          = apvts.getRawParameterValue (ParamID::cloud)->load();
    params.vox            = apvts.getRawParameterValue (ParamID::vox)->load();
    params.realm          = apvts.getRawParameterValue (ParamID::realm)->load();
    params.chaos          = apvts.getRawParameterValue (ParamID::chaos)->load();

    // DREAM macro: morph toward fever-dream targets without replacing knob values in APVTS
    const float d = params.dream;
    auto lerp = [d] (float base, float target) { return base + d * (target - base); };
    params.modDepth    = juce::jmin (1.0f, lerp (params.modDepth,    0.92f));
    params.shimmer     = juce::jmin (1.0f, lerp (params.shimmer,     juce::jmin (1.0f, params.shimmer + 0.45f)));
    params.reverseMix  = lerp (params.reverseMix,  0.85f);
    params.decayColor  = lerp (params.decayColor,  0.55f);
    params.tiltEQ      = lerp (params.tiltEQ,      0.45f);
    params.swim        = juce::jmin (1.0f, lerp (params.swim,        juce::jmax (params.swim, 0.65f)));
    params.glow        = juce::jmin (1.0f, lerp (params.glow,        juce::jmax (params.glow, 0.35f)));
    params.cloud       = juce::jmin (1.0f, lerp (params.cloud,       juce::jmax (params.cloud, 0.25f)));
    params.realm       = juce::jmin (1.0f, lerp (params.realm,       juce::jmax (params.realm, 0.4f)));
    params.shimmerDrift = juce::jmin (1.0f, lerp (params.shimmerDrift, juce::jmax (params.shimmerDrift, 0.5f)));

    // Report reverse-reverb latency (one ping-pong buffer = preDelay ms) to DAW
    const int newLatency = params.reverse
        ? juce::jlimit (1, 15000, juce::jmax (1, (int) (params.preDelay * 0.001 * getSampleRate())))
        : 0;
    if (newLatency != lastReportedLatency)
    {
        setLatencySamples (newLatency);
        lastReportedLatency = newLatency;
    }

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
