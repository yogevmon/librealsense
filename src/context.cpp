// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <array>

#include "sr300.h"
#include "ds5.h"
#include "backend.h"
#include "context.h"
#include "recorder.h"

#define constexpr_support 1

#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#undef constexpr_support
#define constexpr_support 0
#endif
#endif

#if (constexpr_support == 1)
template<unsigned... Is> struct seq{};
template<unsigned N, unsigned... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<unsigned N1, unsigned... I1, unsigned N2, unsigned... I2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2], seq<I1...>, seq<I2...>){
  return {{ a1[I1]..., a2[I2]... }};
}

template<unsigned N1, unsigned N2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2]){
  return concat(a1, a2, gen_seq<N1-1>{}, gen_seq<N2>{});
}

constexpr auto rs_api_version = concat("VERSION: ",RS_API_VERSION_STR);

#endif

namespace rsimpl
{
    context::context(backend_type type, 
                     const char* filename, 
                     const char* section, 
                     rs_recording_mode mode)
    {
        switch(type)
        {
        case backend_type::standard:
            _backend = uvc::create_backend();
            break;
        case backend_type::record:
            _backend = std::make_shared<uvc::record_backend>(uvc::create_backend(), filename, section, mode);
            break;
        case backend_type::playback: 
            _backend = std::make_shared<uvc::playback_backend>(filename, section);
            break;
        default: throw invalid_value_exception(to_string() << "Undefined backend type " << static_cast<int>(type));
        }
    }

    std::vector<std::shared_ptr<device_info>> context::query_devices() const
    {
        std::vector<std::shared_ptr<device_info>> list;

        auto uvc_devices = _backend->query_uvc_devices();
        auto usb_devices = _backend->query_usb_devices();
        auto hid_devices = _backend->query_hid_devices();

        auto sr300_devices = pick_sr300_devices(uvc_devices, usb_devices);
        for (auto&& dev : sr300_devices)
            list.push_back(dev);

        auto ds5_devices = pick_ds5_devices(uvc_devices, usb_devices, hid_devices);
        for (auto&& dev : ds5_devices)
            list.push_back(dev);

        auto recovery_devices = recovery_info::pick_recovery_devices(usb_devices);
        for (auto&& dev : recovery_devices)
            list.push_back(dev);

        return list;
    }
}
