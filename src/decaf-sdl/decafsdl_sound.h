#pragma once
#include <SDL.h>
#include <libdecaf/decaf_sound.h>
#ifdef DOLPHIN
#include <common/log.h>
#include <fmt/format.h>
#else
#include <spdlog/spdlog.h>
#endif
#include <vector>

class DecafSDLSound : public decaf::SoundDriver
{
public:
   DecafSDLSound();
   ~DecafSDLSound() override;

   bool start(unsigned outputRate, unsigned numChannels) override;
   void output(int16_t *samples, unsigned numSamples) override;
   void stop() override;

private:
   std::shared_ptr<spdlog::logger> mLog;

   unsigned mNumChannelsIn;  // Number of channels of data we receive in output()
   unsigned mNumChannelsOut; // Number of channels we send to the audio device
   unsigned mOutputFrameLen; // Number of samples (per channel) in an output frame

   // Output buffer (ring buffer): written by output(), read by SDL callback
   std::vector<int16_t> mOutputBuffer;
   size_t mBufferWritePos; // Index of next sample (array element) to write
   size_t mBufferReadPos;  // Index of next sample (array element) to read

   static void sdlCallback(void *instance, Uint8 *stream, int size);
};
