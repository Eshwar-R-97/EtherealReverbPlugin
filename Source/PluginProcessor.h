#pragma once
#include <JuceHeader.h>
#include "DSPEngine.h"

namespace ParamID
{
    static const juce::String preDelay  { "preDelay"  };
    static const juce::String roomSize  { "roomSize"  };
    static const juce::String decay     { "decay"     };
    static const juce::String damping   { "damping"   };
    static const juce::String diffusion { "diffusion" };
    static const juce::String modRate   { "modRate"   };
    static const juce::String modDepth  { "modDepth"  };
    static const juce::String tiltEQ    { "tiltEQ"    };
    static const juce::String mix       { "mix"       };
    static const juce::String freeze    { "freeze"    };
}

class EtherealReverbProcessor : public juce::AudioProcessor
{
public:
    EtherealReverbProcessor();
    ~EtherealReverbProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName()  const override { return JucePlugin_Name; }
    bool acceptsMidi()            const override { return false; }
    bool producesMidi()           const override { return false; }
    bool isMidiEffect()           const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int  getNumPrograms()                              override { return 1;  }
    int  getCurrentProgram()                           override { return 0;  }
    void setCurrentProgram (int)                       override {}
    const juce::String getProgramName (int)            override { return {}; }
    void changeProgramName (int, const juce::String&)  override {}

    void getStateInformation (juce::MemoryBlock& destData)       override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    DSPEngine dspEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EtherealReverbProcessor)
};
