/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class Wind4Unity1AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Wind4Unity1AudioProcessor();
    ~Wind4Unity1AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //  Wind Methods
    void dstPrepare(const juce::dsp::ProcessSpec& spec);
    void dstProcess(juce::AudioBuffer<float>& buffer);
    void dstUpdateSettings();
    
    //  Global Parameters
    juce::AudioParameterFloat* gain;
        
    //  Distant Wind Parameters
    juce::AudioParameterFloat* dstBPCutoffFreq;
    juce::AudioParameterFloat* dstBPQ;
    juce::AudioParameterFloat* dstAmplitude;
    
    //  Distant Wind DSP Resources
    juce::dsp::Oscillator<float> dstNoise1 {[] (float x) { juce::Random r; return r.nextFloat() * 2.0f - 1.0f; }};
    
    juce::dsp::StateVariableTPTFilter<float> dstBPF;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Wind4Unity1AudioProcessor)
};
