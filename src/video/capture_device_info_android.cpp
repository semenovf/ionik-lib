////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2024.03.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "video/capture_device.hpp"
#include "pfs/fmt.hpp"
#include "pfs/i18n.hpp"
#include <camera/NdkCameraManager.h>
#include <media/NdkImage.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace ionik {
namespace video {

static std::pair<std::string, std::string> image_format_name (std::int32_t id)
{
    switch (id) {
        // 32 bits RGBA format, 8 bits for each of the four channels.
        // V4L2_PIX_FMT_RGBA32
        case AIMAGE_FORMAT_RGBA_8888:
            return std::make_pair("AB24", "32 bits RGBA");

        // 32 bits RGBX format, 8 bits for each of the four channels.  The values
        // of the alpha channel bits are ignored (image is assumed to be opaque)
        // V4L2_PIX_FMT_RGBX32
        case AIMAGE_FORMAT_RGBX_8888:
            return std::make_pair("XB24", "32 bits RGBX");

        // 24 bits RGB format, 8 bits for each of the three channels.
        // V4L2_PIX_FMT_RGB24
        case AIMAGE_FORMAT_RGB_888:
            return std::make_pair("RGB3", "24 bits RGB");

        // 16 bits RGB format, 5 bits for Red channel, 6 bits for Green channel,
        // and 5 bits for Blue channel.
        // V4L2_PIX_FMT_RGB565
        case AIMAGE_FORMAT_RGB_565:
            return std::make_pair("RGBP", "16 bits RGB");

        // 64 bits RGBA format, 16 bits for each of the four channels.
        case AIMAGE_FORMAT_RGBA_FP16:
            return std::make_pair("FP16", "64 bits RGBA");

        // Multi-plane Android YUV 420 format. This format is a generic YCbCr format.
        // ? V4L2_PIX_FMT_YVU420
        case AIMAGE_FORMAT_YUV_420_888:
            return std::make_pair("YV12", "Multi-plane YUV 420");

        // Compressed JPEG format.
        // V4L2_PIX_FMT_JPEG
        case AIMAGE_FORMAT_JPEG:
            return std::make_pair("JPEG", "Compressed JPEG");

        // 16 bits per pixel raw camera sensor image format, usually representing a single-channel
        // Bayer-mosaic image.
        case AIMAGE_FORMAT_RAW16:
            return std::make_pair("RW16", "16 bits per pixel raw camera sensor image");

        // Private raw camera sensor image format, a single channel image with implementation
        // depedent pixel layout.
        case AIMAGE_FORMAT_RAW_PRIVATE:
            return std::make_pair("RWPV", "Private raw camera sensor image");

        // Android 10-bit raw format.
        // This is a single-plane, 10-bit per pixel, densely packed (in each row),
        // unprocessed format, usually representing raw Bayer-pattern images coming
        // from an image sensor.
        case AIMAGE_FORMAT_RAW10:
            return std::make_pair("RW10", "Android 10-bit raw");

        // Android 12-bit raw format.
        // This is a single-plane, 12-bit per pixel, densely packed (in each row),
        // unprocessed format, usually representing raw Bayer-pattern images coming
        // from an image sensor.
        case AIMAGE_FORMAT_RAW12:
            return std::make_pair("RW12", "Android 10-bit raw");

        // Android dense depth image format.
        case AIMAGE_FORMAT_DEPTH16:
            return std::make_pair("DP16", "Android dense depth image");

        // Android sparse depth point cloud format.
        case AIMAGE_FORMAT_DEPTH_POINT_CLOUD:
            return std::make_pair("ADPC", "Android sparse depth point cloud format");

        // Android private opaque image format.
        case AIMAGE_FORMAT_PRIVATE:
            return std::make_pair("APRV", "Android private opaque image");

        // Android Y8 format.
        // Y8 is a planar format comprised of a WxH Y plane only, with each pixel
        // being represented by 8 bits.
        case AIMAGE_FORMAT_Y8:
            return std::make_pair("APY8", "Android private opaque image");

        // Compressed HEIC format.
        case AIMAGE_FORMAT_HEIC:
            return std::make_pair("HEIC", "Compressed HEIC");

        // Depth augmented compressed JPEG format.
        case AIMAGE_FORMAT_DEPTH_JPEG:
            return std::make_pair("HEIC", "Depth augmented compressed JPEG");

        default:
            break;
    }

    return std::make_pair(fmt::format("{:x}", id), fmt::format("Unknown image format {:x}", id));
}

std::vector<capture_device_info> fetch_capture_devices (error * perr)
{
    std::vector<capture_device_info> result;
    error err;

    ACameraIdList * idList = nullptr;
    auto manager = ACameraManager_create();

    if (manager != nullptr) {
        auto status = ACameraManager_getCameraIdList(manager, & idList);

        if (status == ACAMERA_OK) {
            for (int i = 0; i < idList->numCameras && !err; i++) {
                result.emplace_back();
                capture_device_info & cdi = result.back();
                cdi.subsystem     = subsystem_enum::camera2android;
                cdi.readable_name = std::to_string(i);
                cdi.current_pixel_format_index = 0;

                std::map<std::int32_t, std::vector<std::pair<std::int32_t,std::int32_t>>> image_format_resolutions;
                std::vector<std::pair<std::int32_t,std::int32_t>> frame_rates;

                ACameraMetadata * meta = nullptr;
                status = ACameraManager_getCameraCharacteristics(manager, idList->cameraIds[i], & meta);

                if (status == ACAMERA_OK) {
                    ACameraMetadata_const_entry entry;

                    std::array<std::uint32_t, 2> tags = {
                        ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
                        , ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES
                    };

                    for (auto tag: tags) {
                        if (err)
                            break;

                        std::memset(& entry, 0, sizeof(entry));
                        status = ACameraMetadata_getConstEntry(meta, tag, & entry);

                        if (status == ACAMERA_OK) {
                            switch (tag) {
                                case ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS: {
                                    for (int i = 0; i < entry.count; i++) {
                                        auto input = entry.data.i32[i * 4 + 3];

                                        if (input)
                                            continue;

                                        auto pixel_format = entry.data.i32[i * 4 + 0];
                                        auto width = entry.data.i32[i * 4 + 1];
                                        auto height = entry.data.i32[i * 4 + 2];

                                        if (width == 0 || height == 0)
                                            continue;

                                        image_format_resolutions[pixel_format].push_back(std::make_pair(width, height));
                                    }

                                    break;
                                }

                                case ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES: {
                                    for (int i = 0; i < entry.count; i++) {
                                        auto min_fps = entry.data.i32[i * 2 + 0];
                                        auto max_fps = entry.data.i32[i * 2 + 1];

                                        // Ignore too big values
                                        // if (min_fps > 499 || max_fps > 499)
                                        //    continue;

                                        frame_rates.push_back(std::make_pair(min_fps, max_fps));
                                    }

                                    break;
                                }

                                default:
                                    break;
                            }
                        } else {
                            err = error {
                                errc::unsupported
                                , tr::f_("query metadata (ACameraMetadata_getConstEntry) failure"
                                    " with tag: {}, error code: {} (see camera/NdkCameraError.h for error description)"
                                    , tag, status)
                            };

                            return std::vector<capture_device_info>{};
                        }
                    }
                } else {
                    err = error {
                          errc::unsupported
                        , tr::f_("query camera characteristics (ACameraManager_getCameraCharacteristics)"
                            " failure: error code: {} (see camera/NdkCameraError.h for error description)"
                            , status)
                    };
                }

                if (meta != nullptr)
                    ACameraMetadata_free(meta);

                // YUV_420_888

                for (auto pos = image_format_resolutions.cbegin(), last = image_format_resolutions.cend(); pos != last; ++pos) {
                    std::int32_t image_format = pos->first;
                    auto const & resolutions = pos->second;

                    cdi.pixel_formats.emplace_back();
                    pixel_format & pxf = cdi.pixel_formats.back();

                    auto pair = image_format_name(image_format);
                    pxf.name = pair.first;
                    pxf.description = pair.second;

                    for (auto const & pair: resolutions) {
                    }
                }
            }
        } else {
            err = error {
                  errc::unsupported
                , tr::f_("query cameras (ACameraManager_getCameraIdList) failure: error code: {}"
                    " (see camera/NdkCameraError.h for error description)"
                    , status)
            };
        }
    } else {

    }

    if (idList != nullptr)
        ACameraManager_deleteCameraIdList(idList);

    if (manager != nullptr)
        ACameraManager_delete(manager);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return std::vector<capture_device_info>{};
    }

    return result;
}

}} // namespace ionik::video

