#include "al/app/al_App.hpp"
#include "Gamma/Oscillator.h"

// This example shows how to use a Gamma generator in an allolib app.

using namespace al;

struct MyApp : App
{
  gam::Sine<> osc; // A Gamma sine oscillator

  void onCreate() override {
    // Always remember to set the default sampling rate of Gamma!
    // If you don't you will hear nothing!
    gam::sampleRate(audioIO().framesPerSecond());
    osc.freq(440); // Set the frequency of the oscillator
  }

  void onSound(AudioIOData& io) override {
    while (io()) {
      float s = osc(); // Generate next sine wave sample
      s *= 0.1f; // Scale the sample down a bit for output
      io.out(0) = s; // write the signal to channels 0 and 1
      io.out(1) = s;
    }
  }
};

int main()
{
  MyApp app;
  // Enable audio with 2 channels of output.
  app.initAudio(44100, 512, 2, 0);
  app.start();
}
