#include "al/core/sound/al_Dbap.hpp"

namespace al{

Dbap::Dbap(const SpeakerLayout &sl, float focus)
	:	Spatializer(sl), mNumSpeakers(0), mFocus(focus)
{
	mNumSpeakers = mSpeakers.size();
	printf("DBAP Compiled with %d speakers\n", mNumSpeakers);

	for(unsigned int i = 0; i < mNumSpeakers; i++)
	{
		mSpeakerVecs[i] = mSpeakers[i].vec();
		mDeviceChannels[i] = mSpeakers[i].deviceChannel;
	}
}

void Dbap::renderSample(AudioIOData &io, const Pose &listeningPose, const float &sample, const int &frameIndex)
{
	Vec3d relpos = listeningPose.vec();

	//Rotate vector according to listener-rotation
	Quatd srcRot = listeningPose.quat();
	relpos = srcRot.rotate(relpos);
	relpos = Vec4d(relpos.x, relpos.z, relpos.y);
	for (unsigned int i = 0; i < mNumSpeakers; ++i)
	{
		float gain = 1.f;
		Vec3d vec = relpos - mSpeakerVecs[i];
		float dist = vec.mag();
		gain = 1.f / (1.f + dist);
		gain = powf(gain, mFocus);

		io.out(mDeviceChannels[i], frameIndex) += gain*sample;
	}
}

void Dbap::renderBuffer(AudioIOData &io, const Pose &listeningPose, const float *samples, const int &numFrames)
{
	Vec3d relpos = listeningPose.vec();

	//Rotate vector according to listener-rotation
	Quatd srcRot = listeningPose.quat();
	relpos = srcRot.rotate(relpos);
	relpos = Vec4d(relpos.x, relpos.z, relpos.y);

    for (unsigned int k = 0; k < mNumSpeakers; ++k)
	{
		float gain = 1.f;

		Vec3d vec = relpos - mSpeakerVecs[k];
		double dist = vec.mag();
		gain = 1.0 / (1.0 + dist);
		gain = powf(gain, mFocus);

		float * out = io.outBuffer(mDeviceChannels[k]);
		for(int i = 0; i < numFrames; ++i){
			out[i] += gain * samples[i];
		}
    }
}

void Dbap::print(std::ostream &stream) {
    stream << "Using DBAP Panning- need to add panner info for print function" << std::endl;
}

} // al::
