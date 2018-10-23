
#include <iostream>
#include <fstream>
#include <sstream>

#include "al/util/ui/al_Parameter.hpp"
#include "al/core/io/al_File.hpp"

using namespace al;

// Parameter ------------------------------------------------------------------
Parameter::Parameter(std::string parameterName, std::string Group,
                     float defaultValue,
                     std::string prefix,
                     float min,
                     float max) :
    ParameterWrapper<float>(parameterName, Group, defaultValue, prefix, min, max)
{
	mFloatValue = defaultValue;
}

float Parameter::get()
{
	return mFloatValue;
}

void Parameter::setNoCalls(float value, void *blockReceiver)
{
	if (value > mMax) value = mMax;
	if (value < mMin) value = mMin;
	if (mProcessCallback) {
        value = (*mProcessCallback)(value); //, mProcessUdata);
	}
    if (blockReceiver) {
        for(auto cb:mCallbacks) {
            (*cb)(value);
        }
	}

	mFloatValue = value;
}

void Parameter::set(float value)
{
	if (value > mMax) value = mMax;
	if (value < mMin) value = mMin;
    if (mProcessCallback) {
        value = (*mProcessCallback)(value); //, mProcessUdata);
    }
    for(auto cb:mCallbacks) {
        (*cb)(value);
    }
    mFloatValue = value;
}

// ParameterBool ------------------------------------------------------------------
ParameterBool::ParameterBool(std::string parameterName, std::string Group,
                     float defaultValue,
                     std::string prefix,
                     float min,
                     float max) :
    Parameter(parameterName, Group, defaultValue, prefix, min, max)
{
//	mFloatValue = defaultValue;
}


//void ParameterBool::setNoCalls(float value, void *blockReceiver)
//{
//	if (value > mMax) value = mMax;
//	if (value < mMin) value = mMin;
//	if (mProcessCallback) {
//		value = mProcessCallback(value, mProcessUdata);
//	}
//	if (blockReceiver) {
//		for(size_t i = 0; i < mCallbacks.size(); ++i) {
//			if (mCallbacks[i]) {
//				mCallbacks[i](value, this, mCallbackUdata[i], blockReceiver);
//			}
//		}
//	}

//	mFloatValue = value;
//}

//void ParameterBool::set(float value)
//{
//	if (value > mMax) value = mMax;
//	if (value < mMin) value = mMin;
//	if (mProcessCallback) {
//		value = mProcessCallback(value, mProcessUdata);
//	}
//	mFloatValue = value;
//	for(size_t i = 0; i < mCallbacks.size(); ++i) {
//		if (mCallbacks[i]) {
//			mCallbacks[i](value, this, mCallbackUdata[i], NULL);
//		}
//	}
//}


// --------------------- ParameterMeta ------------

ParameterMeta::ParameterMeta(std::string parameterName, std::string group, std::string prefix) :
    mParameterName(parameterName), mGroup(group), mPrefix(prefix)
{
    //TODO: Add better heuristics for slash handling
    if (mPrefix.length() > 0 && mPrefix.at(0) != '/') {
        mFullAddress = "/";
    }
    mFullAddress += mPrefix;
    if (mPrefix.length() > 0 && mPrefix.at(mPrefix.length() - 1) != '/') {
        mFullAddress += "/";
    }
    if (mGroup.length() > 0 && mGroup.at(0) != '/') {
        mFullAddress += "/";
    }
    mFullAddress += mGroup;
    if (mGroup.length() > 0 && mGroup.at(mGroup.length() - 1) != '/') {
        mFullAddress += "/";
    }
    if (mFullAddress.length() == 0) {
        mFullAddress = "/";
    }
    mFullAddress += mParameterName;
}
