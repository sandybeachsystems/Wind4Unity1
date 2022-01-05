/*
  ==============================================================================

    Copyright (c) 2022 - Gordon Webb
 
    This file is part of Wind4Unity1.

    Wind4Unity1 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Wind4Unity1 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wind4Unity1.  If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

//==============================================================================
Wind4Unity1AudioProcessor::Wind4Unity1AudioProcessor()

     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       )
{
    //    Global Parameters
    addParameter(gain = new juce::AudioParameterFloat(
                        "Master Gain", "Master Gain", 0.0f, 1.0f, 0.5f));
    
    //    Distant Rain Parameters
    addParameter(dstBPCutoffFreq = new juce::AudioParameterFloat(
                        "DstCoff", "DistantIntensty", 0.004f, 1000.0f, 10.0f));
    addParameter(dstBPQ = new juce::AudioParameterFloat(
                        "DstQ", "DistantQ", 1.0f, 100.0f, 10.0f));
    addParameter(dstAmplitude = new juce::AudioParameterFloat(
                        "DstAmp", "DistantAmpltd", 0.0001f, 1.5f, 0.75f));
}

Wind4Unity1AudioProcessor::~Wind4Unity1AudioProcessor()
{
}

//==============================================================================
const juce::String Wind4Unity1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Wind4Unity1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Wind4Unity1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Wind4Unity1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Wind4Unity1AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Wind4Unity1AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Wind4Unity1AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Wind4Unity1AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Wind4Unity1AudioProcessor::getProgramName (int index)
{
    return {};
}

void Wind4Unity1AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Wind4Unity1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //    Create DSP Spec
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    //    Prepare Distant Wind
    dstPrepare(spec);
    
}

void Wind4Unity1AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Wind4Unity1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Wind4Unity1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
//    auto totalNumInputChannels  = getTotalNumInputChannels();
//    auto totalNumOutputChannels = getTotalNumOutputChannels();
    buffer.clear();
    
    dstUpdateSettings();
    dstProcess(buffer);

    buffer.applyGain(gain->get());

}

//==============================================================================
bool Wind4Unity1AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Wind4Unity1AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void Wind4Unity1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Wind4Unity1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void Wind4Unity1AudioProcessor::dstPrepare(const juce::dsp::ProcessSpec &spec)
{
    //    Prepare Noise Source
    dstNoise1.prepare(spec);
    
    //    Prepare Filter
    dstBPF.prepare(spec);
    dstBPF.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    dstBPF.setResonance(dstBPQ->get());
    dstBPF.reset();
}

void Wind4Unity1AudioProcessor::dstProcess(juce::AudioBuffer<float> &buffer)
{
    //    Get Buffer info
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    float dstFrameAmp = dstAmplitude->get();
    
    //    Distant Wind DSP Loop
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float output = dstBPF.processSample(ch, dstNoise1.processSample(0.0f));
            buffer.addSample(ch, s, output * dstFrameAmp);
        }
    }
    
}

void Wind4Unity1AudioProcessor::dstUpdateSettings()
{
    //    Update Filter Settings
    dstBPF.setCutoffFrequency(dstBPCutoffFreq->get());
    dstBPF.setResonance(dstBPQ->get());
}




//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Wind4Unity1AudioProcessor();
}
