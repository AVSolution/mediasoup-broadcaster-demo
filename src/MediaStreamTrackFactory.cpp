#define MSC_CLASS "MediaStreamTrackFactory"

#include <iostream>

#include "MediaSoupClientErrors.hpp"
#include "MediaStreamTrackFactory.hpp"
#include "pc/test/fake_audio_capture_module.h"
#include "pc/test/fake_periodic_video_track_source.h"
#include "pc/test/frame_generator_capturer_video_track_source.h"
#include "system_wrappers/include/clock.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "pc/test/vcm_capturer.h"
#include "modules/video_capture/video_capture_factory.h"
#include "test/test_video_capturer.h"

using namespace mediasoupclient;

static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
static rtc::scoped_refptr<FakeAudioCaptureModule> fakeAudioCaptureModule;

/* MediaStreamTrack holds reference to the threads of the PeerConnectionFactory.
 * Use plain pointers in order to avoid threads being destructed before tracks.
 */
static rtc::Thread* networkThread;
static rtc::Thread* signalingThread;
static rtc::Thread* workerThread;

class CapturerTrackSource : public webrtc::VideoTrackSource
{
public:
	static rtc::scoped_refptr<CapturerTrackSource> Create()
	{
		const size_t kWidth  = 640;
		const size_t kHeight = 480;
		const size_t kFps    = 30;
		std::unique_ptr<webrtc::test::VcmCapturer> capturer;
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		  webrtc::VideoCaptureFactory::CreateDeviceInfo());
		if (!info)
		{
			return nullptr;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i)
		{
			capturer = absl::WrapUnique(webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
			if (capturer)
			{
				return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
			}
		}

		return nullptr;
	}

protected:
	explicit CapturerTrackSource(std::unique_ptr<webrtc::test::VcmCapturer> capturer)
	  : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer))
	{
	}

private:
	rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override
	{
		return capturer_.get();
	}
	std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

static void createFactory()
{
	networkThread   = rtc::Thread::Create().release();
	signalingThread = rtc::Thread::Create().release();
	workerThread    = rtc::Thread::Create().release();

	networkThread->SetName("network_thread", nullptr);
	signalingThread->SetName("signaling_thread", nullptr);
	workerThread->SetName("worker_thread", nullptr);

	if (!networkThread->Start() || !signalingThread->Start() || !workerThread->Start())
	{
		MSC_THROW_INVALID_STATE_ERROR("thread start errored");
	}

	webrtc::PeerConnectionInterface::RTCConfiguration config;

	fakeAudioCaptureModule = FakeAudioCaptureModule::Create();
	if (!fakeAudioCaptureModule)
	{
		MSC_THROW_INVALID_STATE_ERROR("audio capture module creation errored");
	}

	factory = webrtc::CreatePeerConnectionFactory(
	  networkThread,
	  workerThread,
	  signalingThread,
	  fakeAudioCaptureModule,
	  webrtc::CreateBuiltinAudioEncoderFactory(),
	  webrtc::CreateBuiltinAudioDecoderFactory(),
	  webrtc::CreateBuiltinVideoEncoderFactory(),
	  webrtc::CreateBuiltinVideoDecoderFactory(),
	  nullptr /*audio_mixer*/,
	  nullptr /*audio_processing*/);

	if (!factory)
	{
		MSC_THROW_ERROR("error ocurred creating peerconnection factory");
	}
}

// Audio track creation.
rtc::scoped_refptr<webrtc::AudioTrackInterface> createAudioTrack(const std::string& label)
{
	if (!factory)
		createFactory();

	cricket::AudioOptions options;
	options.highpass_filter = false;
	options.echo_cancellation = false;

	rtc::scoped_refptr<webrtc::AudioSourceInterface> source = factory->CreateAudioSource(options);

	return factory->CreateAudioTrack(label, source);
}

rtc::scoped_refptr<webrtc::AudioTrackInterface> createCaptureAudioTrack(const std::string& label)
{
	if (!factory)
		createFactory();

	cricket::AudioOptions options;
	options.highpass_filter = false;

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
	  factory->CreateAudioTrack(
	  label, factory->CreateAudioSource(options)));

	return audio_track;
}

// Video track creation.
rtc::scoped_refptr<webrtc::VideoTrackInterface> createVideoTrack(const std::string& /*label*/)
{
	if (!factory)
		createFactory();

	auto* videoTrackSource =
	  new rtc::RefCountedObject<webrtc::FakePeriodicVideoTrackSource>(false /* remote */);

	return factory->CreateVideoTrack(rtc::CreateRandomUuid(), videoTrackSource);
}

//Capture RT Video Track creation.
rtc::scoped_refptr<webrtc::VideoTrackInterface> createCaptureVideoTrack(const std::string& label)
{
	if (!factory)
		createFactory();

	rtc::scoped_refptr<CapturerTrackSource> video_device = CapturerTrackSource::Create();
	if (video_device)
	{
		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
		  factory->CreateVideoTrack(label, video_device));
		return video_track_;
	}

	return nullptr;
}


rtc::scoped_refptr<webrtc::VideoTrackInterface> createSquaresVideoTrack(const std::string& /*label*/)
{
	if (!factory)
		createFactory();

	std::cout << "[INFO] getting frame generator" << std::endl;
	auto* videoTrackSource = new rtc::RefCountedObject<webrtc::FrameGeneratorCapturerVideoTrackSource>(
	  webrtc::FrameGeneratorCapturerVideoTrackSource::Config(), webrtc::Clock::GetRealTimeClock(), false);
	videoTrackSource->Start();

	std::cout << "[INFO] creating video track" << std::endl;
	return factory->CreateVideoTrack(rtc::CreateRandomUuid(), videoTrackSource);
}

namespace ADM
{
void mute(bool muted)
{
	if (fakeAudioCaptureModule)
		muted ? fakeAudioCaptureModule->StopRecording() : fakeAudioCaptureModule->StartRecording();
}

void startPlayout(bool start)
{
}

void setPlayoutVolume(int vol)
{
}
} // namespace ADM
