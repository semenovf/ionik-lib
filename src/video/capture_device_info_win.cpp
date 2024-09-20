////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.04.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "video/capture_device.hpp"

#include <pfs/bits/operationsystem.h>
#include <pfs/fmt.hpp>
#include <pfs/hash_combine.hpp>
#include <pfs/i18n.hpp>
#include <pfs/optional.hpp>
#include <pfs/windows.hpp>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <pfs/log.hpp>

// [SafeRelease](https://learn.microsoft.com/ru-ru/windows/win32/medfound/saferelease)
template <typename T>
void SafeRelease (T ** ppT)
{
    if (*ppT) {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

template <typename T>
std::unique_ptr<T, void (*) (T *)>
make_release_guard (T * p)
{
    std::unique_ptr<T, void (*) (T*)> guard {p, [] (T * p) {
        if (p != nullptr)
            p->Release();
    }};

    return guard;
}

namespace std
{
template <>
struct hash<GUID>
{
    // See https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    std::size_t operator() (GUID const & u) const noexcept
    {
#if defined(PFS__OS_64BITS)
        std::uint64_t hi = static_cast<std::uint64_t>(u.Data1);
        std::uint64_t mid = static_cast<std::uint64_t>(u.Data2) << 48
            | static_cast<std::uint64_t>(u.Data3) << 32;
        std::uint64_t lo = static_cast<std::uint64_t>(u.Data4[0]) << 56
            | static_cast<std::uint64_t>(u.Data4[1]) << 48
            | static_cast<std::uint64_t>(u.Data4[2]) << 40
            | static_cast<std::uint64_t>(u.Data4[3]) << 32
            | static_cast<std::uint64_t>(u.Data4[4]) << 24
            | static_cast<std::uint64_t>(u.Data4[5]) << 16
            | static_cast<std::uint64_t>(u.Data4[6]) <<  8
            | static_cast<std::uint64_t>(u.Data4[7]);

        std::size_t result = 0;
        pfs::hash_combine(result, hi, mid, lo);
        return result;

#elif defined(PFS__OS_32BITS)
        // TODO Implement
#   error "Unsupported operation system"
#else
#   error "Unsupported operation system"
#endif
    }
};

} // namespace std

namespace ionik {
namespace video {

static std::string to_string (IMFActivate * pdev, IID const & guid_key)
{
    LPWSTR buffer = nullptr;
    UINT32 length = 0;
    HRESULT hr = pdev->GetAllocatedString(guid_key, & buffer, & length);

    if (FAILED(hr))
        return std::string{};

    auto result = pfs::windows::utf8_encode(buffer, length);

    CoTaskMemFree(buffer);

    return result;
}

static std::pair<std::string, std::string> video_format (GUID const & subtype)
{
    static std::unordered_map<GUID, std::pair<std::string, std::string>> const __mapping {
          { MFVideoFormat_RGB32  , {"RGB4", "32-bit RGB"} } // V4L2_PIX_FMT_RGB32
        , { MFVideoFormat_ARGB32 , {"BA4 ", "32-bit RGB with alpha channel"} } // V4L2_PIX_FMT_ARGB32 ?
        , { MFVideoFormat_RGB24  , {"RGB3", "24-bit RGB"} }     // V4L2_PIX_FMT_RGB24 ?
        , { MFVideoFormat_RGB555 , {"RGBO", "16-bit RGB 555"} } // V4L2_PIX_FMT_RGB555 ?
        , { MFVideoFormat_RGB565 , {"RGBP", "16-bit RGB 565"} } // V4L2_PIX_FMT_RGB565 ?
        , { MFVideoFormat_RGB8   , {"RGB8", "8-bit RGB"} }

        // Luminance and Depth Formats
        , { MFVideoFormat_L8     , {"L8  ", "8-bit luminance only"} }
        , { MFVideoFormat_L16    , {"L16 ", "16-bit luminance only"} }
        , { MFVideoFormat_D16    , {"D16 ", "16-bit z-buffer depth"} }

        // YUV Formats
        , { MFVideoFormat_AI44   , {"AI44", "AI44 YUV format"} }
        , { MFVideoFormat_AYUV   , {"AYUV", "AYUV YUV format"} }
        , { MFVideoFormat_YUY2   , {"YUY2", "YUY2 YUV format"} }
        , { MFVideoFormat_YVYU   , {"YVYU", "YVYU YUV format"} }
        , { MFVideoFormat_YVU9   , {"YVU9", "YVU9 YUV format"} }
        , { MFVideoFormat_UYVY   , {"UYVY", "UYVY YUV format"} }
        , { MFVideoFormat_NV11   , {"NV11", "NV11 YUV format"} }
        , { MFVideoFormat_NV12   , {"NV12", "NV12 YUV format"} }
#if _MSC_VER >= 1930 // Visual Studio 2022 RTW 17.0
	, { MFVideoFormat_NV21   , {"NV21", "NV21 YUV format"} }
#endif
        , { MFVideoFormat_YV12   , {"YV12", "YV12 YUV format"} }
        , { MFVideoFormat_I420   , {"I420", "I420 YUV format"} }
        , { MFVideoFormat_IYUV   , {"IYUV", "IYUV YUV format"} }
        , { MFVideoFormat_Y210   , {"Y210", "Y210 YUV format"} }
        , { MFVideoFormat_Y216   , {"Y216", "Y216 YUV format"} }
        , { MFVideoFormat_Y410   , {"Y410", "Y410 YUV format"} }
        , { MFVideoFormat_Y416   , {"Y416", "Y416 YUV format"} }
        , { MFVideoFormat_Y41P   , {"Y41P", "Y41P YUV format"} }
        , { MFVideoFormat_Y41T   , {"Y41T", "Y41T YUV format"} }
        , { MFVideoFormat_Y42T   , {"Y42T", "Y42T YUV format"} }
        , { MFVideoFormat_P210   , {"P210", "P210 YUV format"} }
        , { MFVideoFormat_P216   , {"P216", "P216 YUV format"} }
        , { MFVideoFormat_P010   , {"P010", "P010 YUV format"} }
        , { MFVideoFormat_P016   , {"P016", "P016 YUV format"} }
        , { MFVideoFormat_v210   , {"V210", "v210 YUV format"} }
        , { MFVideoFormat_v216   , {"V216", "v216 YUV format"} }
        , { MFVideoFormat_v410   , {"V410", "v410 YUV format"} }
        , { MFVideoFormat_420O   , {"420O", "8-bit per channel planar YUV 4:2:0 video"} }

        // Encoded Video Types
        , { MFVideoFormat_MP43   , {"MP43", "Microsoft MPEG 4 codec version 3"} }
        , { MFVideoFormat_MP4S   , {"MP4S", "ISO MPEG 4 codec version 1"} }
        , { MFVideoFormat_M4S2   , {"M4S2", "MPEG-4 part 2 video (M4S2)"} }
        , { MFVideoFormat_MP4V   , {"MP4V", "MPEG-4 part 2 video (MP4V)"} }
        , { MFVideoFormat_WMV1   , {"WMV1", "Windows Media Video codec version 7"} }
        , { MFVideoFormat_WMV2   , {"WMV2", "Windows Media Video 8 codec"} }
        , { MFVideoFormat_WMV3   , {"WMV3", "Windows Media Video 9 codec"} }
        , { MFVideoFormat_WVC1   , {"WVC1", "SMPTE 421M"} }
        , { MFVideoFormat_MSS1   , {"MSS1", "Windows Media Screen codec version 1"} }
        , { MFVideoFormat_MSS2   , {"MSS2", "Windows Media Video 9 Screen codec"} }
        , { MFVideoFormat_MPG1   , {"MPG1", "MPEG-1 video"} }
        , { MFVideoFormat_DVSL   , {"DVSL", "SD-DVCR"} }
        , { MFVideoFormat_DVSD   , {"DVSD", "SDL-DVCR"} }
        , { MFVideoFormat_DVHD   , {"DVHD", "HD-DVCR"} }
        , { MFVideoFormat_DV25   , {"DV25", "DVCPRO 25"} }
        , { MFVideoFormat_DV50   , {"DV50", "DVCPRO 50"} }
        , { MFVideoFormat_DVH1   , {"DVH1", "DVCPRO 100"} }
        , { MFVideoFormat_DVC    , {"DVC ", "DVC/DV Video"} }
        , { MFVideoFormat_H264   , {"H264", "H.264 video"} }
        , { MFVideoFormat_H265   , {"H265", "H.265 video"} }
        , { MFVideoFormat_MJPG   , {"MJPG", "Motion JPEG"} }
        , { MFVideoFormat_HEVC   , {"HEVC", "HEVC"} }
        , { MFVideoFormat_HEVC_ES, {"HEVS", "HEVS"} }
        , { MFVideoFormat_VP80   , {"VP80", "VP8 video"} }
        , { MFVideoFormat_VP90   , {"VP90", "VP9 video"} }
        , { MFVideoFormat_ORAW   , {"ORAW", ""} }

#if (WINVER >= _WIN32_WINNT_WIN8)
        , { MFVideoFormat_H263, {"H263", "H.263 video"} }
#endif

//#if (WDK_NTDDI_VERSION >= NTDDI_WIN10)
//        , { MFVideoFormat_A2R10G10B10  , {"", ""} }
//        , { MFVideoFormat_A16B16G16R16F, {"", ""} }
//#endif

#if (WDK_NTDDI_VERSION >= NTDDI_WIN10_RS3)
        , { MFVideoFormat_VP10, {"VP10", "VP10"} }
        , { MFVideoFormat_AV1 , {"AV01", "AV1 video"} }
#endif

//#if (NTDDI_VERSION >= NTDDI_WIN10_FE)
//        , { MFVideoFormat_Theora, {"THEO", "Theora"} }
//#endif
    };

    auto pos = __mapping.find(subtype);

    if (pos == __mapping.cend())
        return std::make_pair(std::string{}, std::string{});

    return pos->second;
}

static pfs::optional<pixel_format> make_pixel_format (IMFMediaType * mediaType)
{
    GUID subtype = GUID_NULL;
    
    HRESULT hr = mediaType->GetGUID(MF_MT_SUBTYPE, & subtype);

    if (FAILED(hr))
        return pfs::nullopt;

    auto vf = video_format(subtype);

    if (vf.first.empty())
        return pfs::nullopt; 

    UINT32 width = 0u;
    UINT32 height = 0u;

    hr = MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, & width, & height);

    if (FAILED(hr))
        return pfs::nullopt;

    UINT32 max_num = 0u;
    UINT32 max_denom = 0u;
    UINT32 min_num = 0u;
    UINT32 min_denom = 0u;

    hr = MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE_RANGE_MIN, & min_num, & min_denom);
    hr = MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE_RANGE_MAX, & max_num, & max_denom);

    pixel_format pxf;
    pxf.name = std::move(vf.first);
    pxf.description = std::move(vf.second);
    pxf.discrete_frame_sizes.resize(1);
    pxf.discrete_frame_sizes[0].width = width;
    pxf.discrete_frame_sizes[0].height = height;
    pxf.discrete_frame_sizes[0].frame_rates.resize(1);
    pxf.discrete_frame_sizes[0].frame_rates[0].num = max_num;
    pxf.discrete_frame_sizes[0].frame_rates[0].denom = max_denom;
    pxf.discrete_frame_sizes[0].frame_rates[0].min_num = min_num;
    pxf.discrete_frame_sizes[0].frame_rates[0].min_denom = min_denom;

    return pfs::make_optional(std::move(pxf));
}

static pixel_format * locate_pixel_format (std::vector<pixel_format> & pixel_formats, std::string const & name)
{
    auto pos = std::find_if(pixel_formats.begin(), pixel_formats.end(), [& name] (pixel_format const & pxf) {
        return name == pxf.name;
    });

    if (pos == pixel_formats.end())
        return nullptr;

    return & *pos;
}

static discrete_frame_size * locale_discrete_frame_size (std::vector<discrete_frame_size> & sizes
        , std::uint32_t width, std::uint32_t height)
{
    auto pos = std::find_if(sizes.begin(), sizes.end(), [width, height](discrete_frame_size const & dfs) {
        return dfs.width == width && dfs.height == height;
    });

    if (pos == sizes.end())
        return nullptr;

    return & *pos;
}

IMFSourceReader * create_source_reader (IMFMediaSource * psource, error * perr)
{
    IMFAttributes * pattrs = nullptr;

    HRESULT hr = MFCreateAttributes(& pattrs, 1);
    auto attrs_guard = make_release_guard(pattrs);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error{
              errc::backend_error
           , tr::_("MFCreateAttributes() call failure")
        });

        return nullptr;
    }

    hr = pattrs->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error{
              errc::backend_error
            , tr::_("IMFAttributes::SetUINT32() call failure")
        });

        return nullptr;
    }

    // By default, when the application releases the source reader, the source reader 
    // shuts down the media source by calling IMFMediaSource::Shutdown on the media 
    // source.At that point, the application can no longer use the media source.
    // To change this default behavior, set the MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN 
    // attribute in the pAttributes parameter. If this attribute is TRUE, the application is 
    // responsible for shutting down the media source.

    IMFSourceReader * reader = nullptr;
    hr = MFCreateSourceReaderFromMediaSource(psource, pattrs, & reader);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error{
              errc::backend_error
            , tr::_("MFCreateSourceReaderFromMediaSource() call failure")
        });

        return nullptr;
    }

    return reader;
}

static pfs::optional<pixel_format> current_pixel_format (IMFMediaSource * psource, error * perr)
{
    auto reader = make_release_guard(create_source_reader(psource, perr));

    if (!reader)
        return pfs::nullopt;

    IMFMediaType * current_media_type = nullptr;

    HRESULT hr = reader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        , & current_media_type);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::_("IMFSourceReader::GetCurrentMediaType() call failure")
        });

        return pfs::nullopt;
    }

    auto current_pxf_opt = make_pixel_format(current_media_type);

    if (!current_pxf_opt) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_error
            , tr::_("unsupported current media type for video capture device")
        });

        return pfs::nullopt;
    }

    return current_pxf_opt;
}

static std::vector<pixel_format> enumerate_pixel_formats (IMFMediaSource * psource, error * perr)
{
    auto reader = make_release_guard(create_source_reader(psource, perr));

    if (!reader)
        return std::vector<pixel_format>{};

    std::vector<pixel_format> pixel_formats;

    for (DWORD i = 0;; i++) {
        IMFMediaType * mediaType = nullptr;

        HRESULT hr = reader->GetNativeMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM)
            , i, & mediaType);

        auto media_type_guard = make_release_guard(mediaType);

        if (SUCCEEDED(hr)) {
            auto pxf_opt = make_pixel_format(mediaType);

            if (pxf_opt) {
                auto * pxf_ptr = locate_pixel_format(pixel_formats, pxf_opt->name);

                if (pxf_ptr == nullptr) {
                    pixel_formats.push_back(*pxf_opt);
                } else {
                    auto * dfs_ptr = locale_discrete_frame_size(pxf_ptr->discrete_frame_sizes
                        , pxf_opt->discrete_frame_sizes[0].width, pxf_opt->discrete_frame_sizes[0].height);

                    if (dfs_ptr == nullptr) {
                        pxf_ptr->discrete_frame_sizes.push_back(std::move(pxf_opt->discrete_frame_sizes[0]));
                    } else {
                        dfs_ptr->frame_rates.push_back(pxf_opt->discrete_frame_sizes[0].frame_rates[0]);
                    }
                }
            }
        }

        if (FAILED(hr)) {
            switch (hr) {
                // Not an error
                case MF_E_INVALIDSTREAMNUMBER:
                case MF_E_NO_MORE_TYPES:
                    break;

                default:
                    pfs::throw_or(perr, error { 
                          errc::backend_error
                        , tr::_("IMFSourceReader::GetNativeMediaType() call failure")
                    });

                    return std::vector<pixel_format>{};
            }

            break;
        }
    }

    return pixel_formats;
}

static pfs::optional<capture_device_info> create_capture_device (IMFActivate * pdev, error * perr)
{
    IMFMediaSource * psource = nullptr;
     
    HRESULT hr = pdev->ActivateObject(IID_PPV_ARGS(& psource));
    auto source_guard = make_release_guard(psource);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::_("IMFActivate::ActivateObject() call failure")
        });

        return pfs::nullopt;
    }

    capture_device_info info;
    info.subsystem = subsystem_enum::windows;
    info.id = to_string(pdev, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
    info.readable_name = to_string(pdev, MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
    info.orientation = 0;
    info.pixel_formats = enumerate_pixel_formats(psource, perr);

    bool current_pixel_format_match = false;
    auto current_pixel_format_opt = current_pixel_format(psource, perr);

    psource->Shutdown();

    if (!current_pixel_format_opt)
        return pfs::nullopt;

    for (std::size_t i = 0; i < info.pixel_formats.size(); i++) {
        if (info.pixel_formats[i].description == current_pixel_format_opt->description) {
            current_pixel_format_match = true;
            info.current_pixel_format_index = i;
            info.current_frame_size.width  = current_pixel_format_opt->discrete_frame_sizes[0].width;
            info.current_frame_size.height = current_pixel_format_opt->discrete_frame_sizes[0].height;
            break;
        }
    }

    if (!current_pixel_format_match) {
        pfs::throw_or(perr, error {
              pfs::errc::unexpected_error
            , tr::f_("none of the pixel formats match current one for video capture device: {}"
                , info.readable_name)
        });

        return pfs::nullopt;
    }

    return pfs::make_optional<capture_device_info>(std::move(info));
}

static std::vector<capture_device_info> enumerate_devices (IMFAttributes * pattrs, error * perr)
{
    std::vector<capture_device_info> result;

    IMFActivate ** pdevices = nullptr;
    UINT32 count = 0;

    HRESULT hr = MFEnumDeviceSources(pattrs, & pdevices, & count);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error{
              errc::backend_error
            , tr::_("MFEnumDeviceSources() call failure")
        });

        return std::vector<capture_device_info>{};
    }

    if (count == 0)
        return std::vector<capture_device_info>{};

    for (UINT32 i = 0; i < count; i++) {
        if (pdevices[i] != nullptr) {
            auto devopt = create_capture_device(pdevices[i], perr);

            if (devopt)
                result.push_back(std::move(*devopt));
        }
    }

    for (DWORD i = 0; i < count; i++)
        SafeRelease(& pdevices[i]);

    CoTaskMemFree(pdevices);

    return result;
}

std::vector<capture_device_info> fetch_capture_devices (error * perr)
{
    struct com_library_initializer 
    {
        com_library_initializer () 
        {
            CoInitialize(nullptr);
            MFStartup(MF_VERSION);
        }

        ~com_library_initializer () 
        { 
            MFShutdown();
            CoUninitialize();
        }
    } com_library_initializer;

    std::vector<capture_device_info> result;

    IMFAttributes * pattrs = nullptr;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(& pattrs, 1);
    auto attrs_guard = make_release_guard(pattrs);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error { 
              errc::backend_error
            , tr::_("MFCreateAttributes() call failure")
        });

        return std::vector<capture_device_info>{};
    }

    // Source type: video capture devices
    hr = pattrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE
        , MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (FAILED(hr)) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::_("IMFAttributes::SetGUID() call failure")
        });

        return std::vector<capture_device_info>{};
    }

    return enumerate_devices(pattrs, perr);
}

}} // namespace ionik::video
