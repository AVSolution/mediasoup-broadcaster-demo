#ifndef MSC_TEST_MEDIA_STREAM_TRACK_FACTORY_HPP
#define MSC_TEST_MEDIA_STREAM_TRACK_FACTORY_HPP

#include "api/media_stream_interface.h"

rtc::scoped_refptr<webrtc::AudioTrackInterface> createAudioTrack(const std::string& label);

rtc::scoped_refptr<webrtc::VideoTrackInterface> createVideoTrack(const std::string& label);

rtc::scoped_refptr<webrtc::VideoTrackInterface> createSquaresVideoTrack(const std::string& label);

rtc::scoped_refptr<webrtc::VideoTrackInterface> createCaptureVideoTrack(const std::string& label);

rtc::scoped_refptr<webrtc::AudioTrackInterface> createCaptureAudioTrack(const std::string& label);

namespace ADM
{
void mute(bool muted) ;
void startPlayout(bool start) ;
void setPlayoutVolume(int vol) ;
}//namespace ADM

#endif
